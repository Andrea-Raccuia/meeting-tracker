from pypdf import PdfReader
import pandas as pd
import ctypes
import os
import re
from datetime import date, timedelta
import calendar
from matplotlib import pyplot as plt
import tkinter as tk
from tkinter import ttk
from tkinterdnd2 import DND_FILES, TkinterDnD

# ---------------------------------------------------------------------------
# CORE PROCESSING FUNCTION
# ---------------------------------------------------------------------------

def process_pdf(pdf_path: str):
    """
    Full pipeline: reads the PDF, extracts names and assignments,
    assigns weekly dates, builds and saves the attendance CSV,
    then displays the analysis charts.
    """

    csv_path = r"C:\Users\Utente\Downloads\presenze.csv"

    # -- Extract raw text from the PDF ---------------------------------------
    reader = PdfReader(pdf_path)
    with open(r"C:\Users\Utente\Downloads\output.txt", "w", encoding="utf-8") as f:
        for page in reader.pages:
            testo = page.extract_text()
            if testo:
                f.write(testo)
                f.write("\n\n")

    with open(r"C:\Users\Utente\Downloads\output.txt", "r", encoding="utf-8") as f:
        testo = f.read()

    # -- Parse month and year from the PDF filename --------------------------
    month_names = {
        "gennaio": 1, "febbraio": 2, "marzo": 3, "aprile": 4,
        "maggio": 5, "giugno": 6, "luglio": 7, "agosto": 8,
        "settembre": 9, "ottobre": 10, "novembre": 11, "dicembre": 12
    }

    base_name = os.path.basename(pdf_path).lower()

    found_month = None
    for name, num in month_names.items():
        if name in base_name:
            found_month = num
            break

    found_year = int(re.search(r"\d{4}", base_name).group())

    # -- Compute the Mondays of the month (one per weekly meeting) -----------
    first_day = date(found_year, found_month, 1)
    last_day  = date(found_year, found_month, calendar.monthrange(found_year, found_month)[1])

    weeks = []
    day = first_day
    while day <= last_day:
        if day.weekday() == 0:  # Monday
            weeks.append(day)
        day += timedelta(days=1)

    # -- Load the student name database --------------------------------------
    df_names = pd.read_csv(r"C:\Users\Utente\Desktop\proj dat_an\studenti.csv")
    db_str = ",".join(df_names["nome"].str.strip())

    # -- Load the parser DLL -------------------------------------------------
    os.add_dll_directory(r"C:\msys64\mingw64\bin")
    lib = ctypes.CDLL(r"C:\Users\Utente\Desktop\proj dat_an\parser.dll")

    lib.risultato_finale_c.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    lib.risultato_finale_c.restype  = ctypes.c_char_p

    lib.estrai_parti_c.argtypes = [ctypes.c_char_p]
    lib.estrai_parti_c.restype  = ctypes.c_char_p

    # -- Call DLL: extract resolved names and assignment labels --------------
    name_list = lib.risultato_finale_c(
        testo.encode("utf-8"),
        db_str.encode("utf-8")
    ).decode("utf-8").strip(",").split(",")

    part_list = lib.estrai_parti_c(
        testo.encode("utf-8")
    ).decode("utf-8").strip(",").split(",")

    # -- Assign a date to each row using "Lettura biblica" as week marker ----
    date_list = []
    current_week = weeks[0]
    week_idx = 0

    for part in part_list:
        if "Lettura biblica" in part and week_idx < len(weeks):
            current_week = weeks[week_idx]
            week_idx += 1
        date_list.append(current_week.strftime("%d/%m/%Y"))

    # -- Trim all lists to the same length -----------------------------------
    def trim_lists(*lists):
        min_len = min(len(l) for l in lists)
        return [l[:min_len] for l in lists]

    name_list, part_list, date_list = trim_lists(name_list, part_list, date_list)

    # -- Build DataFrame and save to CSV -------------------------------------
    df = pd.DataFrame({
        "name": name_list,
        "part": part_list,
        "date": date_list
    })
    df.to_csv(csv_path, mode='w', header=True, index=False)

    # -- Analysis ------------------------------------------------------------
    df_total = pd.read_csv(csv_path)

    # Total appearances per student
    total_appearances = df_total["name"].value_counts()

    # Student with the most appearances
    top_student = total_appearances.idxmax()
    top_count   = total_appearances.max()

    # Most frequent assignment per student
    most_common_part = (
        df_total.groupby("name")["part"]
        .agg(lambda x: x.value_counts().idxmax())
    )

    # Full summary table: appearances + most common assignment
    summary = pd.DataFrame({
        "appearances":   total_appearances,
        "most_common_part": most_common_part
    }).sort_values("appearances", ascending=False)

    print("=" * 50)
    print(f"Most active student: {top_student} ({top_count} appearances)")
    print("=" * 50)
    print(summary.to_string())

    # -- Charts --------------------------------------------------------------
    # Overall attendance chart
    fig1, ax1 = plt.subplots(figsize=(10, 5))
    ax1.bar(total_appearances.index, total_appearances.values, color="steelblue")
    ax1.set_title("Total attendance")
    ax1.set_ylabel("Appearances")
    ax1.tick_params(axis='x', rotation=45)
    plt.tight_layout()

    # One chart per week
    for week in df_total["date"].unique():
        df_week = df_total[df_total["date"] == week]
        presenze = df_week["name"].value_counts()

        fig, ax = plt.subplots(figsize=(10, 5))
        ax.bar(presenze.index, presenze.values, color="darkorange")
        ax.set_title(f"Attendance — week of {week}")
        ax.set_ylabel("Appearances")
        ax.tick_params(axis='x', rotation=45)
        plt.tight_layout()

    plt.show()

# ---------------------------------------------------------------------------
# DRAG & DROP INTERFACE
# Built by Claude
# ---------------------------------------------------------------------------

class App(TkinterDnD.Tk):
    def __init__(self):
        super().__init__()
        self.title("Attendance Analyzer")
        self.geometry("500x300")
        self.resizable(False, False)
        self._build_ui()

    def _build_ui(self):
        # Title label
        tk.Label(
            self, text="Attendance Analyzer",
            font=("Helvetica", 16, "bold")
        ).pack(pady=20)

        # Credit label
        tk.Label(
            self, text="Built by Andrea Raccuia and Claude (Anthropic)",
            font=("Helvetica", 9), fg="gray"
        ).pack()

        # Drop zone
        self.drop_label = tk.Label(
            self,
            text="Drag & drop a PDF here",
            font=("Helvetica", 12),
            bg="#dce8f5", relief="ridge",
            width=40, height=5
        )
        self.drop_label.pack(pady=20)
        self.drop_label.drop_target_register(DND_FILES)
        self.drop_label.dnd_bind("<<Drop>>", self._on_drop)

        # Status label
        self.status = tk.Label(self, text="", fg="green", font=("Helvetica", 10))
        self.status.pack()

    def _on_drop(self, event):
        # Clean the path returned by tkinterdnd2 (removes curly braces on Windows)
        path = event.data.strip().strip("{}")
        if not path.lower().endswith(".pdf"):
            self.status.config(text="Error: please drop a PDF file.", fg="red")
            return
        self.status.config(text=f"Processing: {os.path.basename(path)}...", fg="blue")
        self.update()
        try:
            process_pdf(path)
            self.status.config(text="Done! Charts generated.", fg="green")
        except Exception as e:
            self.status.config(text=f"Error: {e}", fg="red")

if __name__ == "__main__":
    app = App()
    app.mainloop()