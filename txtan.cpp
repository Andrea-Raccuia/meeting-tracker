#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

// Compile as DLL:
// g++ -shared -o parser.dll -fPIC -static-libgcc -static-libstdc++ txtan.cpp

// ---------------------------------------------------------------------------
// UTILITY FUNCTIONS
// ---------------------------------------------------------------------------

// Splits a string into a vector of words using space as delimiter
vector<string> estrai_parole(string& s) {
    vector<string> parole;
    bool c = false; // tracks whether we are currently inside a word
    for (int i = 0; i < s.size(); i++) {
        if (s[i] == ' ') {
            c = false; continue;
        }
        if (!c) {
            parole.push_back(string(1, s[i])); // start a new word
            c = true;
            continue;
        }
        parole[parole.size()-1].push_back(s[i]); // append char to current word
    }
    return parole;
}

// Keywords that signal a student speaker label in the transcript
vector<string> parole_indizio = {"Studente:", "Studente/Assistente:"};

// Sentinel words that mark the end of a speaker section
string A = "VITA";
string B = "EFFICACI";

// Returns true if two strings are exactly equal (case-sensitive)
bool is_eq(string& p, string& s) {
    if (p.size() != s.size()) return false;
    for (int i = 0; i < p.size(); i++) if (p[i] != s[i]) return false;
    return true;
}

// Returns true if character c is a digit
bool is_n(char c) {
    string num = "0123456789";
    for (auto& n : num) if (c == n) return true;
    return false;
}

// Returns true if character c is found anywhere in string s
bool is_p(string s, char c) {
    for (auto& n : s) if (n == c) return true;
    return false;
}

// Splits a string into two parts at the first occurrence of character c,
// skipping the separator itself
pair<string, string> estrai_div(string& s, char c) {
    string n1, n2;
    int i = 0;
    for (; i < s.size(); i++) {
        if (s[i] == c) break;
        n1.push_back(s[i]);
    }
    i++; // skip the separator
    for (; i < s.size(); i++) {
        n2.push_back(s[i]);
    }
    return {n1, n2};
}

// ---------------------------------------------------------------------------
// PART EXTRACTION
// ---------------------------------------------------------------------------

// Scans the word list for speaker labels and extracts the assignment name
// (e.g. "3. Lettura biblica") that precedes each label.
// Duplicates the entry for dual-speaker labels (Studente/Assistente).
vector<string> estrai_parti(vector<string>& parole) {
    vector<string> parti;
    for (int i = 0; i < (int)parole.size(); i++) {

        if (is_eq(parole[i], parole_indizio[0]) || is_eq(parole[i], parole_indizio[1])) {
            int ien = i - 1;
            string parte = "";
            string numero = "";
            vector<string> tmp;

            // walk backwards to collect the assignment name words
            while (ien >= 0) {
                if (is_n(parole[ien][0])) { numero = parole[ien]; break; } // stop at the item number
                tmp.push_back(parole[ien]);
                ien--;
            }

            // words were collected in reverse order, flip them back
            for (int j = (int)tmp.size() - 1; j >= 0; j--) {
                parte += tmp[j];
                if (j != 0) parte += " ";
            }

            // strip the duration "(X min)" from the end if present
            auto pos = parte.rfind('(');
            if (pos != string::npos && pos > 0) parte = parte.substr(0, pos - 1);

            string risultato = numero + " " + parte;

            parti.push_back(risultato);
            if (is_eq(parole[i], parole_indizio[1]))
                parti.push_back(risultato); // duplicate entry for the assistant
        }
    }
    return parti;
}

// ---------------------------------------------------------------------------
// NAME EXTRACTION
// ---------------------------------------------------------------------------

// Scans the word list for speaker labels and extracts the names that follow.
// Returns a list of (name, type) pairs:
//   type 0 = single speaker (Studente)
//   type 1 = dual speaker   (Studente/Assistente)
vector<pair<string, int>> estrai_nomi(vector<string>& parole) {
    vector<pair<string,int>> nomi;
    for (int i = 0; i < parole.size(); i++) {

        // Case 1: single student label "Studente:"
        if (is_eq(parole[i], parole_indizio[0])) {
            i++;
            string nome = "";
            while (i < parole.size()) {
                // stop at a number or at a sentinel section keyword
                if (is_n(parole[i][0]) || is_eq(parole[i], A) || is_eq(parole[i], B)) break;
                nome += parole[i];
                i++;
            }
            nomi.push_back({nome, 0});
        }

        // Case 2: dual speaker label "Studente/Assistente:"
        if (i < parole.size() && is_eq(parole[i], parole_indizio[1])) {
            i++;
            bool c = false; // true once we have passed the '/' separator
            string nome1 = "", nome2 = "";
            while (i < parole.size()) {
                // stop at a number or at a sentinel section keyword
                if (is_n(parole[i][0]) || is_eq(parole[i], A) || is_eq(parole[i], B)) break;
                if (is_p(parole[i], '/')) {
                    // word contains '/': split and assign each half to the respective name
                    pair<string,string> coppia = estrai_div(parole[i], '/');
                    nome1 += coppia.first;
                    nome2 += coppia.second;
                    c = true;
                    i++;
                    continue;
                }
                if (!c) { nome1 += parole[i]; i++; } // before '/'
                if (c)  { nome2 += parole[i]; i++; } // after '/'
            }
            nomi.push_back({nome1, 1});
            nomi.push_back({nome2, 1});
        }
    }
    return nomi;
}

// ---------------------------------------------------------------------------
// SIMILARITY & NAME RESOLUTION
// ---------------------------------------------------------------------------

// Returns the maximum value in a float vector
float max(vector<float> a) {
    float m = 0.0f;
    for (auto& n : a) if (n > m) m = n;
    return m;
}

// Returns the index of the maximum value in a float vector
int maxi(vector<float> a) {
    float m = 0.0f;
    int mi = 0;
    for (int i = 0; i < a.size(); i++) {
        if (a[i] > m) { m = a[i]; mi = i; }
    }
    return mi;
}

// Computes a similarity score between strings a and b.
// Finds the longest common substring across all (i,j) starting pairs,
// then returns (longest match length / a.size()) * 100 as a percentage.
float per_sim(string a, string b) {
    vector<float> n;
    for (int i = 0; i < (int)a.size(); i++) {
        for (int j = 0; j < (int)b.size(); j++) {
            float nn = 0;
            int ti = i, tj = j;
            while (ti < (int)a.size() && tj < (int)b.size() && a[ti] == b[tj]) {
                nn++; ti++; tj++;
            }
            n.push_back(nn);
        }
    }
    return max(n) / (float)a.size() * 100.0f;
}

// Splits each entry in lista into a (first name, last name) pair
// using '.' or ' ' as the separator
vector<pair<string, string>> nome_cognome(vector<pair<string, int>>& lista) {
    vector<pair<string, string>> nomi_e_cognomi;
    for (auto& n : lista) {
        if (is_p(n.first, '.'))      nomi_e_cognomi.push_back(estrai_div(n.first, '.'));
        else if (is_p(n.first, ' ')) nomi_e_cognomi.push_back(estrai_div(n.first, ' '));
    }
    return nomi_e_cognomi;
}

// Wraps a vector of strings into (name, 0) pairs for uniform processing
vector<pair<string, int>> mod_in(vector<string> db) {
    vector<pair<string, int>> vreturn;
    for (auto& n : db) vreturn.push_back({n, 0});
    return vreturn;
}

// Extracts only the last name from each (first, last) pair
vector<string> cognomi_s(vector<pair<string, string>>& sn) {
    vector<string> sm;
    for (auto& n : sn) sm.push_back(n.second);
    return sm;
}

// Returns the indices of all db entries whose last name best matches the given surname
vector<int> n_p(vector<string> cognomi, string s) {
    vector<float> val;
    for (auto& n : cognomi) val.push_back(per_sim(n, s));
    float num = max(val);
    vector<int> indx;
    for (int i = 0; i < val.size(); i++) if (val[i] == num) indx.push_back(i);
    return indx;
}

// Filters the db to the entries whose last name best matches the given surname
vector<string> filtra_cognomi(string cognome, vector<string> db) {
    vector<string> vect_r;
    vector<pair<string, int>> db_s = mod_in(db);
    vector<pair<string, string>> lista_v = nome_cognome(db_s);
    vector<string> s_c_v = cognomi_s(lista_v);
    vector<int> indx = n_p(s_c_v, cognome);
    for (int i = 0; i < indx.size(); i++) vect_r.push_back(db[indx[i]]);
    return vect_r;
}

// Resolves a partial/abbreviated name to the best full-name match in the db.
// First filters candidates by last name similarity, then scores by first name.
string estrai_il_nome(string& nom_p, vector<string> db) {
    vector<string> vect_r = filtra_cognomi(nom_p, db);

    // extract the first-name portion from the input token
    string nome_parte;
    if (is_p(nom_p, '.'))      nome_parte = estrai_div(nom_p, '.').first;
    else if (is_p(nom_p, ' ')) nome_parte = estrai_div(nom_p, ' ').first;
    else                       nome_parte = nom_p;

    // score each candidate's first name against the extracted portion
    vector<string> nomi_filtrati;
    for (auto& v : vect_r) nomi_filtrati.push_back(estrai_div(v, ' ').first);

    vector<float> scores;
    for (auto& n : nomi_filtrati) scores.push_back(per_sim(nome_parte, n));

    return vect_r[maxi(scores)];
}

// Resolves all extracted name tokens to their full db matches
vector<string> nomi_estr(vector<pair<string, int>> nomi, vector<string> db) {
    vector<string> dreturn;
    for (auto& n : nomi) dreturn.push_back(estrai_il_nome(n.first, db));
    return dreturn;
}

// Joins a vector of strings into a single comma-separated string
string trasf(vector<string> stringa) {
    string nnn;
    for (auto& n : stringa) { nnn += n; nnn += ","; }
    return nnn;
}

// ---------------------------------------------------------------------------
// PIPELINE ENTRY POINTS
// ---------------------------------------------------------------------------

// Takes a raw transcript string, extracts speaker names,
// resolves them against the db, and returns them as a comma-separated list
string risultato_finale(string sss, vector<string> db) {
    vector<string> pr = estrai_parole(sss);
    vector<pair<string, int>> nomi = estrai_nomi(pr);
    vector<string> nn = nomi_estr(nomi, db);
    return trasf(nn);
}

// ---------------------------------------------------------------------------
// C INTERFACE (called via ctypes from Python)
// ---------------------------------------------------------------------------

// Parses a comma-separated string of names into a vector<string>
vector<string> parse_db(const char* db_str) {
    vector<string> db;
    string current;
    for (int i = 0; db_str[i] != '\0'; i++) {
        if (db_str[i] == ',') {
            if (!current.empty()) { db.push_back(current); current.clear(); }
        } else {
            current.push_back(db_str[i]);
        }
    }
    if (!current.empty()) db.push_back(current);
    return db;
}

extern "C" {
    // Resolves speaker names from the transcript against the name database.
    // Input:  raw transcript text, comma-separated name database
    // Output: comma-separated list of resolved full names
    const char* risultato_finale_c(const char* input, const char* db_str) {
        static std::string result;
        vector<string> db = parse_db(db_str);
        result = risultato_finale(std::string(input), db);
        return result.c_str();
    }

    // Extracts assignment names and numbers from the transcript.
    // Output: comma-separated list of entries like "3. Lettura biblica"
    const char* estrai_parti_c(const char* input) {
        static std::string result;
        std::string s = std::string(input);
        vector<string> pr = estrai_parole(s);
        vector<string> parti = estrai_parti(pr);
        result = "";
        for (auto& p : parti) { result += p; result += ","; }
        return result.c_str();
    }
}