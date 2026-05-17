🧠 Task

Reimplement the behavior of C’s standard `printf()` function using only low-level tools and variadic functions.

This project recreates a simplified version of `printf`, supporting multiple format specifiers and handling variable-length argument lists via `stdarg.h`. Output is written directly to `stdout` using low-level functions (e.g., `write`), with no access to standard formatted I/O functions like `printf`, `fprintf`, or `sprintf`.

---

💡 Description

The `my_printf()` function processes a format string that includes standard directives (like `%d`, `%s`, `%x`, etc.) and replaces them with corresponding values passed as additional arguments. This custom implementation supports:

* `%s` – String
* `%S` – Uppercase string (custom extension)
* `%d` – Signed integer
* `%u` – Unsigned integer
* `%c` – Character
* `%p` – Pointer (in hex)
* `%o` – Octal integer
* `%x` / `%X` – Hexadecimal (lower/uppercase)

To build this function, I studied:

* Variadic functions with `va_start`, `va_arg`, and `va_end`
* Manual number formatting (decimal, octal, hex)
* Pointer handling and hexadecimal conversion
* Custom format string parsing and modular design

Let’s just say... I now deeply respect the original designers of `printf()`.

---

🔧 Installation

To compile the project, simply run:

```bash
make
```

> This will produce an executable named `my_printf`.

---

🚀 Usage

Run the program with a hardcoded test case:

```bash
./my_printf
```

Example output from the internal test:

```text
Hello -123 456 HI 0x... e E
```

You can modify the `main()` function to test additional format strings and arguments.

---

🧪 Features Implemented

* ✅ Modular handlers for each format type
* ✅ Manual base conversion (octal, hex)
* ✅ Custom `%S` specifier to uppercase strings
* ✅ Pointer formatting in hex with `0x` prefix
* ✅ Null-safe string handling
* ✅ No use of `printf`, `sprintf`, or other disallowed functions

---

🔥 Challenges Faced

> “Dear God, please let this be the hardest project in the bootcamp.”
> – Me, after 10 hours of debugging `%x` and pointer formatting.

This project was a deep dive into low-level C programming, memory representation, recursion, and string formatting logic. It pushed me to understand how tools we take for granted (like `printf`) are built from scratch.

---

📌 Requirements Met

* No use of global/static variables
* No use of forbidden functions (`printf`, `fprintf`, etc.)
* No logic in macros
* Memory-safe (checkable via `-fsanitize=address`)

---

🧠 What I Learned

* How `printf` works internally
* How to manage and traverse variadic arguments
* How to convert and print integers in multiple bases
* How to design modular, extensible code for complex format parsing
