🧠 Task

In computational linguistics and probability theory, an **n-gram** is a contiguous sequence of items (characters, syllables, words, etc.) from a given text or speech sample. N-gram models are widely used in applications such as predictive text, search engines, speech recognition, and machine translation.

This program implements a basic **unigram (1-gram)** frequency counter — it analyzes input strings and outputs the frequency of each character in **alphanumerical order**.

---

📄 Description

To solve the problem, I took the following steps:

1. **Researched** the concept of n-grams to ensure accurate understanding.
2. **Parsed command-line arguments** to collect input strings.
3. **Counted character occurrences** using a fixed-size array to represent ASCII characters.
4. **Sorted and printed** character counts in alphanumerical order (based on ASCII value).

The result is a clean C implementation that mimics core aspects of string parsing and frequency counting — similar to features you'd find in scripting languages like Python or JavaScript, but done manually for a better understanding of memory and control flow.

---

🧰 Installation

Make sure you have `gcc` or any standard C compiler installed.

Build the project using:

```bash
make
```

---

🚀 Usage

Run the program with one or more strings as arguments:

```bash
./my_ngram "hello world" "foo bar"
```

Expected output (example):

```
 :2
a:1
b:1
d:1
e:1
f:1
h:1
l:3
o:4
r:2
w:1
```

Each line shows:

* The **character** (`:` separated)
* Its **number of occurrences** across all input strings

Characters are listed in ASCII (alphanumerical) order.

---
