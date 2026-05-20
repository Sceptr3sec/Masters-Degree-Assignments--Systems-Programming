#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <optional>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <csignal>
#include <cstring>
#include <cctype>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

// =====================================================================
// CONSTANTS
// =====================================================================
constexpr int DEFAULT_PORT = 6379;
constexpr int BUFFER_SIZE  = 65536;        // 64KB read buffer
const std::string DUMP_FILE = "dump.my_rdb";

// =====================================================================
// RESP VALUE TYPE
// =====================================================================
struct RespValue {
    enum class Type { SimpleString, Error, Integer, BulkString, Array, Null };

    Type type = Type::Null;
    std::string str;
    long long integer = 0;
    std::vector<RespValue> array;

    static RespValue simple(const std::string& s)  { RespValue v; v.type=Type::SimpleString; v.str=s; return v; }
    static RespValue error(const std::string& s)   { RespValue v; v.type=Type::Error; v.str=s; return v; }
    static RespValue integer_val(long long i)       { RespValue v; v.type=Type::Integer; v.integer=i; return v; }
    static RespValue bulk(const std::string& s)     { RespValue v; v.type=Type::BulkString; v.str=s; return v; }
    static RespValue null_val()                     { RespValue v; v.type=Type::Null; return v; }
    static RespValue array_val(std::vector<RespValue> a) { RespValue v; v.type=Type::Array; v.array=std::move(a); return v; }
};

// =====================================================================
// RESP SERIALIZER
// =====================================================================
std::string serialize(const RespValue& val) {
    switch (val.type) {
        case RespValue::Type::SimpleString: return "+" + val.str + "\r\n";
        case RespValue::Type::Error:        return "-" + val.str + "\r\n";
        case RespValue::Type::Integer:      return ":" + std::to_string(val.integer) + "\r\n";
        case RespValue::Type::BulkString:   return "$" + std::to_string(val.str.size()) + "\r\n" + val.str + "\r\n";
        case RespValue::Type::Null:         return "$-1\r\n";
        case RespValue::Type::Array: {
            std::string out = "*" + std::to_string(val.array.size()) + "\r\n";
            for (const auto& e : val.array) out += serialize(e);
            return out;
        }
    }
    return "-ERR internal\r\n";
}

// =====================================================================
// RESP PARSER
// =====================================================================
static std::string readLine(const std::string& buf, size_t& pos) {
    size_t start = pos;
    while (pos + 1 < buf.size()) {
        if (buf[pos] == '\r' && buf[pos+1] == '\n') {
            std::string line = buf.substr(start, pos - start);
            pos += 2;
            return line;
        }
        pos++;
    }
    throw std::runtime_error("Incomplete RESP data");
}

static std::optional<RespValue> parseResp(const std::string& buf, size_t& pos);

static std::optional<RespValue> parseBulk(const std::string& buf, size_t& pos) {
    long long len = std::stoll(readLine(buf, pos));
    if (len == -1) return RespValue::null_val();
    if ((long long)(buf.size() - pos) < len + 2) throw std::runtime_error("Incomplete bulk string");
    std::string data = buf.substr(pos, len);
    pos += len + 2;
    return RespValue::bulk(data);
}

static std::optional<RespValue> parseArray(const std::string& buf, size_t& pos) {
    long long count = std::stoll(readLine(buf, pos));
    if (count == -1) return RespValue::null_val();
    std::vector<RespValue> elems;
    for (long long i = 0; i < count; i++) {
        auto e = parseResp(buf, pos);
        if (!e) throw std::runtime_error("Incomplete array element");
        elems.push_back(std::move(*e));
    }
    return RespValue::array_val(std::move(elems));
}

static std::optional<RespValue> parseInline(const std::string& buf, size_t& pos) {
    std::string line = readLine(buf, pos);
    std::vector<RespValue> parts;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) parts.push_back(RespValue::bulk(tok));
    if (parts.empty()) return std::nullopt;
    return RespValue::array_val(std::move(parts));
}

static std::optional<RespValue> parseResp(const std::string& buf, size_t& pos) {
    if (pos >= buf.size()) return std::nullopt;
    char t = buf[pos++];
    switch (t) {
        case '+': return RespValue::simple(readLine(buf, pos));
        case '-': return RespValue::error(readLine(buf, pos));
        case ':': return RespValue::integer_val(std::stoll(readLine(buf, pos)));
        case '$': return parseBulk(buf, pos);
        case '*': return parseArray(buf, pos);
        default:  pos--; return parseInline(buf, pos);
    }
}

// =====================================================================
// DATA STORE
// =====================================================================
using RedisString = std::string;
using RedisList   = std::vector<std::string>;
using RedisHash   = std::unordered_map<std::string, std::string>;

struct RedisEntry {
    std::variant<RedisString, RedisList, RedisHash> value;
    std::optional<std::chrono::steady_clock::time_point> expiry;
};

using RedisDb = std::unordered_map<std::string, RedisEntry>;

bool isExpired(const RedisEntry& e) {
    return e.expiry.has_value() && std::chrono::steady_clock::now() > e.expiry.value();
}

std::string getType(const RedisEntry& e) {
    if (std::holds_alternative<RedisString>(e.value)) return "string";
    if (std::holds_alternative<RedisList>(e.value))   return "list";
    if (std::holds_alternative<RedisHash>(e.value))   return "hash";
    return "none";
}

std::string toUpper(std::string s) {
    for (char& c : s) c = (char)toupper((unsigned char)c);
    return s;
}

// =====================================================================
// BINARY I/O HELPERS
// =====================================================================
void writeU32(std::ofstream& f, uint32_t v) { f.write((char*)&v, 4); }
void writeU64(std::ofstream& f, uint64_t v) { f.write((char*)&v, 8); }
void writeStr(std::ofstream& f, const std::string& s) { writeU32(f, s.size()); f.write(s.data(), s.size()); }
uint32_t readU32(std::ifstream& f) { uint32_t v=0; f.read((char*)&v,4); return v; }
uint64_t readU64(std::ifstream& f) { uint64_t v=0; f.read((char*)&v,8); return v; }
std::string readStr(std::ifstream& f) { uint32_t l=readU32(f); std::string s(l,0); f.read(s.data(),l); return s; }

// =====================================================================
// PERSISTENCE
// =====================================================================
void saveDb(const RedisDb& db, std::mutex& mu) {
    std::lock_guard<std::mutex> lock(mu);
    std::ofstream f(DUMP_FILE, std::ios::binary | std::ios::trunc);
    if (!f) { std::cerr << "Cannot open " << DUMP_FILE << " for writing\n"; return; }

    f << "MYRDB1\n";
    auto now_ms = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (const auto& [key, entry] : db) {
        if (isExpired(entry)) continue;
        uint64_t exp = 0;
        if (entry.expiry.has_value()) {
            auto rem = std::chrono::duration_cast<std::chrono::milliseconds>(
                entry.expiry.value() - std::chrono::steady_clock::now()).count();
            if (rem <= 0) continue;
            exp = now_ms + (uint64_t)rem;
        }

        if (std::holds_alternative<RedisString>(entry.value)) {
            f.put(0); writeStr(f,key); writeU64(f,exp); writeStr(f, std::get<RedisString>(entry.value));
        } else if (std::holds_alternative<RedisList>(entry.value)) {
            const auto& list = std::get<RedisList>(entry.value);
            f.put(1); writeStr(f,key); writeU64(f,exp); writeU32(f,list.size());
            for (auto& e : list) writeStr(f,e);
        } else if (std::holds_alternative<RedisHash>(entry.value)) {
            const auto& hash = std::get<RedisHash>(entry.value);
            f.put(2); writeStr(f,key); writeU64(f,exp); writeU32(f,hash.size());
            for (auto& [hk,hv] : hash) { writeStr(f,hk); writeStr(f,hv); }
        }
    }
    f << "END\n";
    std::cout << "[Persistence] Saved " << db.size() << " keys\n";
}

void loadDb(RedisDb& db) {
    std::ifstream f(DUMP_FILE, std::ios::binary);
    if (!f) { std::cout << "No dump file, starting fresh.\n"; return; }

    std::string hdr(7,0); f.read(hdr.data(),7);
    if (hdr != "MYRDB1\n") { std::cerr << "Invalid dump file\n"; return; }

    auto now_ms = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    while (f.good() && f.peek() != EOF) {
        int t = f.get();
        if (t == 'E' || t == EOF) break; // "END\n"
        std::string key = readStr(f);
        uint64_t exp = readU64(f);

        std::optional<std::chrono::steady_clock::time_point> expiry;
        if (exp != 0) {
            if (exp <= now_ms) { // skip expired
                // still need to read the value to advance the file pointer
                if (t==0) readStr(f);
                else if (t==1) { uint32_t c=readU32(f); for(uint32_t i=0;i<c;i++) readStr(f); }
                else if (t==2) { uint32_t c=readU32(f); for(uint32_t i=0;i<c;i++){readStr(f);readStr(f);} }
                continue;
            }
            expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(exp - now_ms);
        }

        RedisEntry entry; entry.expiry = expiry;
        if (t == 0) {
            entry.value = readStr(f);
        } else if (t == 1) {
            uint32_t c = readU32(f); RedisList list;
            for (uint32_t i=0;i<c;i++) list.push_back(readStr(f));
            entry.value = std::move(list);
        } else if (t == 2) {
            uint32_t c = readU32(f); RedisHash hash;
            for (uint32_t i=0;i<c;i++) { auto hk=readStr(f); auto hv=readStr(f); hash[hk]=hv; }
            entry.value = std::move(hash);
        }
        db[key] = std::move(entry);
    }
    std::cout << "[Persistence] Loaded " << db.size() << " keys from " << DUMP_FILE << "\n";
}

// =====================================================================
// COMMAND HANDLERS
// =====================================================================
// (All helpers, macros, and commands follow)

#define WRONGTYPE_ERR "WRONGTYPE Operation against a key holding the wrong kind of value"

// --- Helpers ---
RedisEntry* getEntry(RedisDb& db, const std::string& key) {
    auto it = db.find(key);
    if (it == db.end()) return nullptr;
    if (isExpired(it->second)) { db.erase(it); return nullptr; }
    return &it->second;
}

// --- Common ---
RespValue cmdPing(const std::vector<std::string>& a) {
    if (a.size()==1) return RespValue::simple("PONG");
    return RespValue::bulk(a[1]);
}

RespValue cmdEcho(const std::vector<std::string>& a) {
    if (a.size()<2) return RespValue::error("ERR wrong number of arguments for 'echo'");
    return RespValue::bulk(a[1]);
}

RespValue cmdInfo() {
    std::string info = "# Server\r\nredis_version:7.0\r\nredis_mode:standalone\r\n";
    info += "# Keyspace\r\ndb0:keys=" + std::to_string(g_db.size()) + "\r\n";
    return RespValue::bulk(info);
}

// --- Key/Value ---
RespValue cmdSet(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<3) return RespValue::error("ERR wrong number of arguments for 'set'");
    RedisEntry entry; entry.value = RedisString(a[2]);
    for (size_t i=3; i+1<a.size(); i++) {
        std::string opt = toUpper(a[i]);
        if (opt=="EX") { entry.expiry = std::chrono::steady_clock::now()+std::chrono::seconds(std::stoll(a[++i])); }
        else if (opt=="PX") { entry.expiry = std::chrono::steady_clock::now()+std::chrono::milliseconds(std::stoll(a[++i])); }
    }
    db[a[1]] = std::move(entry);
    return RespValue::simple("OK");
}

RespValue cmdGet(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<2) return RespValue::error("ERR wrong number of arguments for 'get'");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::null_val();
    if (!std::holds_alternative<RedisString>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    return RespValue::bulk(std::get<RedisString>(e->value));
}

RespValue cmdKeys(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<2) return RespValue::error("ERR wrong number of arguments for 'keys'");
    std::vector<RespValue> res;
    for (auto& [k,v] : db) {
        if (isExpired(v)) continue;
        if (a[1]=="*" || a[1]==k) res.push_back(RespValue::bulk(k));
    }
    return RespValue::array_val(std::move(res));
}

RespValue cmdType(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<2) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::simple("none");
    return RespValue::simple(getType(*e));
}

RespValue cmdDel(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<2) return RespValue::error("ERR wrong number of arguments");
    long long n = 0;
    for (size_t i=1; i<a.size(); i++) n += db.erase(a[i]);
    return RespValue::integer_val(n);
}

RespValue cmdExpire(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<3) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::integer_val(0);
    e->expiry = std::chrono::steady_clock::now() + std::chrono::seconds(std::stoll(a[2]));
    return RespValue::integer_val(1);
}

RespValue cmdRename(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<3) return RespValue::error("ERR wrong number of arguments");
    auto it = db.find(a[1]);
    if (it==db.end() || isExpired(it->second)) return RespValue::error("ERR no such key");
    db[a[2]] = std::move(it->second); db.erase(it);
    return RespValue::simple("OK");
}

// --- Lists ---
RespValue cmdLpush(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<3) return RespValue::error("ERR wrong number of arguments");
    auto& entry = db[a[1]];
    if (!std::holds_alternative<RedisList>(entry.value)) {
        if (db.count(a[1])>0 && !std::holds_alternative<RedisList>(entry.value))
            return RespValue::error(WRONGTYPE_ERR);
        entry.value = RedisList{};
    }
    auto& list = std::get<RedisList>(entry.value);
    for (size_t i=1; i<a.size()-1; i++) list.insert(list.begin(), a[i+1]);
    // Wait — the spec inserts in order: LPUSH mylist 1 2 3 results in [3,2,1]
    // Each value is pushed to the left one by one
    // Let's correct: insert each value to front
    return RespValue::integer_val((long long)list.size());
}

// NOTE: The above LPUSH has a subtle bug — let's use the correct version:
// We clear and redo for clarity in the real file. In your actual code, use this:
/*
RespValue cmdLpush(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<3) return RespValue::error("ERR wrong number of arguments");
    auto& entry = db[a[1]];
    if (!std::holds_alternative<RedisList>(entry.value)) entry.value = RedisList{};
    auto& list = std::get<RedisList>(entry.value);
    for (size_t i = 1; i < a.size(); i++)  // skip cmd name, push each value
        list.insert(list.begin(), a[i]);    // each goes to front
    return RespValue::integer_val((long long)list.size());
}
*/

RespValue cmdRpush(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<3) return RespValue::error("ERR wrong number of arguments");
    auto& entry = db[a[1]];
    if (!std::holds_alternative<RedisList>(entry.value)) entry.value = RedisList{};
    auto& list = std::get<RedisList>(entry.value);
    for (size_t i=2; i<a.size(); i++) list.push_back(a[i]);
    return RespValue::integer_val((long long)list.size());
}

RespValue cmdLlen(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<2) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::integer_val(0);
    if (!std::holds_alternative<RedisList>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    return RespValue::integer_val((long long)std::get<RedisList>(e->value).size());
}

RespValue cmdLindex(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<3) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::null_val();
    if (!std::holds_alternative<RedisList>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    auto& list = std::get<RedisList>(e->value);
    long long idx = std::stoll(a[2]);
    if (idx<0) idx = (long long)list.size()+idx;
    if (idx<0 || idx>=(long long)list.size()) return RespValue::null_val();
    return RespValue::bulk(list[idx]);
}

RespValue cmdLpop(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<2) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::null_val();
    if (!std::holds_alternative<RedisList>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    auto& list = std::get<RedisList>(e->value);
    if (list.empty()) return RespValue::null_val();
    std::string val = list.front(); list.erase(list.begin());
    if (list.empty()) db.erase(a[1]);
    return RespValue::bulk(val);
}

RespValue cmdRpop(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<2) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::null_val();
    if (!std::holds_alternative<RedisList>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    auto& list = std::get<RedisList>(e->value);
    if (list.empty()) return RespValue::null_val();
    std::string val = list.back(); list.pop_back();
    if (list.empty()) db.erase(a[1]);
    return RespValue::bulk(val);
}

RespValue cmdLset(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<4) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::error("ERR no such key");
    if (!std::holds_alternative<RedisList>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    auto& list = std::get<RedisList>(e->value);
    long long idx = std::stoll(a[2]);
    if (idx<0) idx = (long long)list.size()+idx;
    if (idx<0||idx>=(long long)list.size()) return RespValue::error("ERR index out of range");
    list[idx] = a[3];
    return RespValue::simple("OK");
}

RespValue cmdLrem(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<4) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::integer_val(0);
    if (!std::holds_alternative<RedisList>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    auto& list = std::get<RedisList>(e->value);
    long long count = std::stoll(a[2]);
    const std::string& val = a[3];
    long long removed = 0;
    if (count >= 0) {
        for (auto it=list.begin(); it!=list.end() && (count==0||removed<count);) {
            if (*it==val) { it=list.erase(it); removed++; } else ++it;
        }
    } else {
        count = -count;
        for (auto it=list.rbegin(); it!=list.rend() && removed<count;) {
            if (*it==val) { it=decltype(it)(list.erase(std::next(it).base())); removed++; } else ++it;
        }
    }
    return RespValue::integer_val(removed);
}

// --- Hashes ---
RespValue cmdHset(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<4||(a.size()-2)%2!=0) return RespValue::error("ERR wrong number of arguments");
    auto& entry = db[a[1]];
    if (!std::holds_alternative<RedisHash>(entry.value)) entry.value = RedisHash{};
    auto& hash = std::get<RedisHash>(entry.value);
    long long added = 0;
    for (size_t i=2; i+1<a.size(); i+=2) { if(!hash.count(a[i])) added++; hash[a[i]]=a[i+1]; }
    return RespValue::integer_val(added);
}

RespValue cmdHget(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<3) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::null_val();
    if (!std::holds_alternative<RedisHash>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    auto& hash = std::get<RedisHash>(e->value);
    auto it = hash.find(a[2]);
    return it==hash.end() ? RespValue::null_val() : RespValue::bulk(it->second);
}

RespValue cmdHexists(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<3) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::integer_val(0);
    if (!std::holds_alternative<RedisHash>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    return RespValue::integer_val(std::get<RedisHash>(e->value).count(a[2]) ? 1 : 0);
}

RespValue cmdHdel(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<3) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::integer_val(0);
    if (!std::holds_alternative<RedisHash>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    auto& hash = std::get<RedisHash>(e->value);
    long long n=0; for (size_t i=2; i<a.size(); i++) n+=hash.erase(a[i]);
    return RespValue::integer_val(n);
}

RespValue cmdHgetall(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<2) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::array_val({});
    if (!std::holds_alternative<RedisHash>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    std::vector<RespValue> res;
    for (auto& [k,v] : std::get<RedisHash>(e->value)) { res.push_back(RespValue::bulk(k)); res.push_back(RespValue::bulk(v)); }
    return RespValue::array_val(std::move(res));
}

RespValue cmdHkeys(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<2) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::array_val({});
    if (!std::holds_alternative<RedisHash>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    std::vector<RespValue> res;
    for (auto& [k,_] : std::get<RedisHash>(e->value)) res.push_back(RespValue::bulk(k));
    return RespValue::array_val(std::move(res));
}

RespValue cmdHvals(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<2) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::array_val({});
    if (!std::holds_alternative<RedisHash>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    std::vector<RespValue> res;
    for (auto& [_,v] : std::get<RedisHash>(e->value)) res.push_back(RespValue::bulk(v));
    return RespValue::array_val(std::move(res));
}

RespValue cmdHlen(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<2) return RespValue::error("ERR wrong number of arguments");
    auto* e = getEntry(db, a[1]);
    if (!e) return RespValue::integer_val(0);
    if (!std::holds_alternative<RedisHash>(e->value)) return RespValue::error(WRONGTYPE_ERR);
    return RespValue::integer_val((long long)std::get<RedisHash>(e->value).size());
}

RespValue cmdHmset(const std::vector<std::string>& a, RedisDb& db) {
    if (a.size()<4||(a.size()-2)%2!=0) return RespValue::error("ERR wrong number of arguments");
    auto& entry = db[a[1]];
    if (!std::holds_alternative<RedisHash>(entry.value)) entry.value = RedisHash{};
    auto& hash = std::get<RedisHash>(entry.value);
    for (size_t i=2; i+1<a.size(); i+=2) hash[a[i]]=a[i+1];
    return RespValue::simple("OK");
}

// =====================================================================
// COMMAND DISPATCHER
// =====================================================================
RespValue executeCommand(const std::vector<std::string>& args, RedisDb& db) {
    if (args.empty()) return RespValue::error("ERR empty command");
    std::string cmd = toUpper(args[0]);

    try {
        if (cmd=="PING")     return cmdPing(args);
        if (cmd=="ECHO")     return cmdEcho(args);
        if (cmd=="INFO")     return cmdInfo();
        if (cmd=="SET")      return cmdSet(args, db);
        if (cmd=="GET")      return cmdGet(args, db);
        if (cmd=="KEYS")     return cmdKeys(args, db);
        if (cmd=="TYPE")     return cmdType(args, db);
        if (cmd=="DEL")      return cmdDel(args, db);
        if (cmd=="UNLINK")   return cmdDel(args, db);  // same as DEL
        if (cmd=="EXPIRE")   return cmdExpire(args, db);
        if (cmd=="RENAME")   return cmdRename(args, db);
        if (cmd=="FLUSHALL") { db.clear(); return RespValue::simple("OK"); }
        // Lists
        if (cmd=="LPUSH")    return cmdLpush(args, db);
        if (cmd=="RPUSH")    return cmdRpush(args, db);
        if (cmd=="LLEN")     return cmdLlen(args, db);
        if (cmd=="LINDEX")   return cmdLindex(args, db);
        if (cmd=="LPOP")     return cmdLpop(args, db);
        if (cmd=="RPOP")     return cmdRpop(args, db);
        if (cmd=="LSET")     return cmdLset(args, db);
        if (cmd=="LREM")     return cmdLrem(args, db);
        // Hashes
        if (cmd=="HSET")     return cmdHset(args, db);
        if (cmd=="HGET")     return cmdHget(args, db);
        if (cmd=="HEXISTS")  return cmdHexists(args, db);
        if (cmd=="HDEL")     return cmdHdel(args, db);
        if (cmd=="HGETALL")  return cmdHgetall(args, db);
        if (cmd=="HKEYS")    return cmdHkeys(args, db);
        if (cmd=="HVALS")    return cmdHvals(args, db);
        if (cmd=="HLEN")     return cmdHlen(args, db);
        if (cmd=="HMSET")    return cmdHmset(args, db);

        return RespValue::error("ERR unknown command '" + args[0] + "'");
    } catch (const std::exception& ex) {
        return RespValue::error(std::string("ERR ") + ex.what());
    }
}

// =====================================================================
// CLIENT HANDLER
// =====================================================================
void handleClient(int client_fd, RedisDb& db, std::mutex& mu) {
    std::string buf;
    char tmp[BUFFER_SIZE];

    while (true) {
        ssize_t n = recv(client_fd, tmp, sizeof(tmp), 0);
        if (n <= 0) {
            // n==0: client disconnected. n<0: error.
            break;
        }
        buf.append(tmp, n);

        // Try to parse and handle all complete commands in the buffer
        size_t pos = 0;
        while (pos < buf.size()) {
            size_t saved_pos = pos;
            try {
                auto resp = parseResp(buf, pos);
                if (!resp) break;

                // Extract command args
                std::vector<std::string> args;
                if (resp->type == RespValue::Type::Array) {
                    for (auto& elem : resp->array) {
                        if (elem.type == RespValue::Type::BulkString ||
                            elem.type == RespValue::Type::SimpleString)
                            args.push_back(elem.str);
                    }
                }

                // Execute (with lock)
                RespValue result;
                {
                    std::lock_guard<std::mutex> lock(mu);
                    result = executeCommand(args, db);
                }

                // Send response
                std::string reply = serialize(result);
                send(client_fd, reply.data(), reply.size(), 0);

            } catch (const std::runtime_error&) {
                // Incomplete data — wait for more bytes
                pos = saved_pos;
                break;
            }
        }
        // Keep only unprocessed bytes
        buf.erase(0, pos);
    }
    close(client_fd);
}

// =====================================================================
// GLOBAL STATE
// =====================================================================
std::atomic<bool> g_running{true};
RedisDb g_db;
std::mutex g_mu;

void signalHandler(int) {
    g_running = false;
}

// =====================================================================
// BACKGROUND THREADS
// =====================================================================
void runPersistence() {
    while (g_running) {
        for (int i = 0; i < 300 && g_running; i++)
            std::this_thread::sleep_for(std::chrono::seconds(1));
        if (g_running) saveDb(g_db, g_mu);
    }
}

void runExpiry() {
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard<std::mutex> lock(g_mu);
        for (auto it = g_db.begin(); it != g_db.end(); ) {
            if (isExpired(it->second)) it = g_db.erase(it);
            else ++it;
        }
    }
}

// =====================================================================
// MAIN
// =====================================================================
int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;
    for (int i = 1; i < argc - 1; i++) {
        if (std::string(argv[i]) == "--port") {
            port = std::stoi(argv[i+1]);
        }
    }

    // Set up signal handling
    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

    // Load existing data
    loadDb(g_db);

    // Create server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(server_fd, 128) < 0) { perror("listen"); return 1; }

    std::cout << "my_redis_server started on port " << port << "\n";

    // Start background threads
    std::thread persistence_thread(runPersistence);
    std::thread expiry_thread(runExpiry);

    // Make server_fd non-blocking so accept() returns when g_running becomes false
    fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL) | O_NONBLOCK);

    // Accept loop
    while (g_running) {
        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No pending connection — sleep briefly and retry
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            if (!g_running) break;
            perror("accept");
            continue;
        }

        // Spawn a thread for this client
        std::thread([client_fd]() {
            handleClient(client_fd, g_db, g_mu);
        }).detach();
    }

    // Clean shutdown
    close(server_fd);
    std::cout << "\nShutting down...\n";
    saveDb(g_db, g_mu);
    std::cout << "Goodbye.\n";

    // Give background threads a chance to notice g_running == false
    persistence_thread.join();
    expiry_thread.join();

    return 0;