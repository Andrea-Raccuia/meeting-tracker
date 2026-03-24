# 📋 meeting-tracker

> Automated attendance tracking from meeting transcripts — built with C++, Python, and Pandas.

---

## What it does

**meeting-tracker** takes a PDF transcript of a weekly meeting, extracts the names of participants and their assigned parts, maps them to a date, and produces a full attendance CSV with charts.

No manual data entry. Drop a PDF, get a report.

---

## How it works

```
PDF transcript
     │
     ▼
pypdf (text extraction)
     │
     ▼
parser.dll (C++ engine)
  ├── extracts speaker names
  ├── resolves abbreviations → full names (fuzzy similarity)
  └── extracts assignment labels
     │
     ▼
Pandas (data assembly + analysis)
     │
     ▼
Matplotlib (attendance charts)
```

The C++ parser is compiled as a shared library (`.dll`) and called from Python via `ctypes`.

---

## Features

- 📄 **PDF parsing** — extracts raw text from multi-page transcripts
- 🔍 **Fuzzy name resolution** — matches abbreviated names (e.g. `A. Rossi`) to full names in a student database using a custom longest-common-substring similarity algorithm
- 🗓️ **Automatic date assignment** — detects weekly meeting dates from the filename and assigns them to each row
- 📊 **Attendance analysis** — total appearances per student, most frequent assignment per student, weekly breakdown charts
- 🖱️ **Drag & drop GUI** — built with Tkinter + TkinterDnD2, no command line needed

---

## Tech stack

| Layer | Technology |
|---|---|
| PDF reading | Python `pypdf` |
| Text parsing | C++ (compiled to `.dll`) |
| C++ ↔ Python bridge | `ctypes` |
| Data processing | `pandas` |
| Visualization | `matplotlib` |
| GUI | `tkinter` + `tkinterdnd2` |

---

## Project structure

```
meeting-tracker/
├── txtan.cpp          # C++ parser source
├── parser.dll         # Compiled shared library (Windows)
├── main.py            # Python pipeline + GUI
├── studenti.csv       # Student name database
└── README.md
```

---

## Getting started

### 1. Compile the DLL (Windows + MinGW)

```bash
g++ -shared -o parser.dll -fPIC -static-libgcc -static-libstdc++ txtan.cpp
```

### 2. Install Python dependencies

```bash
pip install pypdf pandas matplotlib tkinterdnd2
```

### 3. Run

```bash
python main.py
```

Then drag and drop a PDF transcript onto the window.

---

## Output

- `presenze.csv` — full attendance table with columns: `name`, `part`, `date`
- Charts displayed automatically: total attendance + one chart per week

---

## Author

**Andrea Raccuia** — 15 years old, Italy 🇮🇹

Second-year student at ITIS (Electronics & Electrotechnics).
Building projects at the intersection of systems programming and data science.

- 🐙 GitHub: [@andrearacc10-cell](https://github.com/andrearacc10-cell)

---

*Built as part of a personal data science + systems programming portfolio.*
