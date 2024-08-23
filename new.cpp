#define Uses_TKeys
#define Uses_TApplication
#define Uses_TEvent
#define Uses_TRect
#define Uses_TDialog
#define Uses_TStaticText
#define Uses_TButton
#define Uses_TMenuBar
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TStatusLine
#define Uses_TStatusItem
#define Uses_TStatusDef
#define Uses_TDeskTop
#define Uses_TInputLine
#define Uses_TWindow
#define Uses_TScrollBar
#define Uses_TScroller
#define Uses_TRadioButtons
#define Uses_TSItem
#include <iostream>
#include <tvision/tv.h>
#include <string>
#include <sqlite3.h>
#include <filesystem>
#include <cstdio>
#include <sstream>
#include <vector>
#include <clocale>

sqlite3 *baza;

const int cmNowyTowar       = 201;
const int cmPokaz           = 202;
const int cmUsunTowar       = 203;
const int cmEdytujTowar     = 204;
const int cmPrzeterminowane = 205;
const int cmOProgramie      = 206;

short nrOkna                = 0;
short nrOknaPrzeterminowane = 0;

struct TTowar
{
    std::string nr, nazwa, cena, ilosc, data, wartosc;
};

class TOkno : public TWindow
{
    public:
        TOkno(const TRect& bounds, const char *title, short number, std::vector<std::string> linie);
};

class TWnetrze : public TScroller
{
    private:
        std::vector<std::string> m_Linie;
    public:
        TWnetrze(const TRect& bounds, TScrollBar *HScrollbar, TScrollBar *VScrollbar, std::vector<std::string> linie);
        virtual void draw();
};

TWnetrze::TWnetrze(const TRect& bounds, TScrollBar *HScrollbar, TScrollBar *VScrollbar, std::vector<std::string> linie) : TScroller(bounds, HScrollbar, VScrollbar)
{
    this->m_Linie = linie;
    growMode = gfGrowHiX | gfGrowHiY;
    options = options | ofFramed;
    setLimit(linie.at(1).length(), linie.size());
}

void TWnetrze::draw()
{
    ushort kolor = getColor(0x0301);
    for (int i = 0; i < size.y; ++i) {
        TDrawBuffer b;
        b.moveChar(0, ' ', kolor, size.x);
        int j = delta.y + i;
        if (j < m_Linie.size()) b.moveStr(0, m_Linie.at(j).substr(delta.x).c_str(), kolor);
        else b.moveStr(0, "", kolor);
        writeLine(0, i, size.x, 1, b);
    }
}


TOkno::TOkno(const TRect& bounds, const char *title, short number, std::vector<std::string> linie) : TWindow(bounds, title, number), TWindowInit(&TOkno::initFrame)
{
    TScrollBar *vScrollbar = standardScrollBar(sbVertical | sbHandleKeyboard);
    TScrollBar *hScrollbar = standardScrollBar(sbHorizontal | sbHandleKeyboard);
    TRect r = getClipRect();
    r.grow(-1, -1);
    insert(new TWnetrze(r, hScrollbar, vScrollbar, linie));
}

class TMagazynApp : public TApplication
{
public:
    TMagazynApp();

    virtual void handleEvent( TEvent& event );
    static TMenuBar *initMenuBar(TRect r);
    static TStatusLine *initStatusLine(TRect r);
private:
    void nowyTowar();
    void pokazMagazyn();
    void usunTowar();
    void edytujTowar();
    void przeterminowane();
    void oProgramie();
    std::string dialogSortowania(bool przeterminowanie);
};

TMagazynApp::TMagazynApp() :
    TProgInit( &TMagazynApp::initStatusLine,
               &TMagazynApp::initMenuBar,
               &TMagazynApp::initDeskTop
             )
{
    std::setlocale(LC_NUMERIC, "en_US.UTF-8"); // ustawienie regionalne powodujące, że kropka staje się separatorem dziesiętnym
}

void TMagazynApp::handleEvent(TEvent& event)
{
    TApplication::handleEvent(event);
    if (event.what == evCommand) switch (event.message.command) {
    case cmNowyTowar:
        nowyTowar();
        clearEvent(event);
        break;
    case cmPokaz:
        pokazMagazyn();
        clearEvent(event);
        break;
    case cmUsunTowar:
        usunTowar();
        clearEvent(event);
        break;
    case cmEdytujTowar:
        edytujTowar();
        clearEvent(event);
        break;
    case cmPrzeterminowane:
        przeterminowane();
        clearEvent(event);
        break;
    case cmOProgramie:
        oProgramie();
        clearEvent(event);
        break;
    default: break;
    }
}

TMenuBar *TMagazynApp::initMenuBar(TRect r)
{
    r.b.y = r.a.y+1;
    return new TMenuBar(r,
        *new TSubMenu("~M~agazyn", kbAltM) +
            *new TMenuItem("~W~yświetl", cmPokaz, kbF1, hcNoContext, "F1") +
            *new TMenuItem("~P~rzeterminowane", cmPrzeterminowane, kbF2, hcNoContext, "F2") +
            newLine() +
            *new TMenuItem("Zakończ (~X~)", cmQuit, kbAltX, hcNoContext, "Alt-X") +
        *new TSubMenu("~T~owar", kbAltT) +
            *new TMenuItem("~N~owy...", cmNowyTowar, kbF3, hcNoContext, "F3") +
            *new TMenuItem("~E~dytuj...", cmEdytujTowar, kbF4, hcNoContext, "F4") +
            *new TMenuItem("~U~suń...", cmUsunTowar, kbF5, hcNoContext, "F5") + 
        *new TSubMenu("~I~nne", kbAltI) +
            *new TMenuItem("~O~ programie...", cmOProgramie, kbAltO)
    );
}

TStatusLine *TMagazynApp::initStatusLine(TRect r)
{
    r.a.y = r.b.y-1;
    return new TStatusLine(r,
        *new TStatusDef(0, 0xFFFF) +
            *new TStatusItem("~Alt-X~ Zakończ", kbAltX, cmQuit)
    );
}

void TMagazynApp::nowyTowar()
{
    struct {
        char nazwa[128];
        char cena[11], ilosc[7];
        char rok[5], miesiac[3], dzien[3];
    } nowyTowarDialog;
    strcpy(nowyTowarDialog.nazwa, "");
    strcpy(nowyTowarDialog.ilosc, "");
    strcpy(nowyTowarDialog.cena , "");
    strcpy(nowyTowarDialog.dzien, "");
    strcpy(nowyTowarDialog.miesiac, "");
    strcpy(nowyTowarDialog.rok, "");

    TDialog *d = new TDialog(TRect(3, 3, 40, 17), "Nowy towar");

    d->insert(new TStaticText(TRect(2, 2, 8, 3), "Nazwa:"));
    d->insert(new TInputLine(TRect(2, 3, 24, 4), 128));

    d->insert(new TStaticText(TRect(2, 4, 8, 5), "Cena:"));
    d->insert(new TInputLine(TRect(2, 5, 12, 6), 11));

    d->insert(new TStaticText(TRect(2, 6, 8, 7), "Ilość:"));
    d->insert(new TInputLine(TRect(2, 7, 9, 8), 7));

    d->insert(new TStaticText(TRect(2, 8, 8, 9), "Data:"));
    d->insert(new TInputLine(TRect(2, 9, 8, 10), 5));

    d->insert(new TStaticText(TRect(8, 9, 9, 10), "-"));
    d->insert(new TInputLine(TRect(9, 9, 13, 10), 3));

    d->insert(new TStaticText(TRect(13, 9, 14, 10), "-"));
    d->insert(new TInputLine(TRect(14, 9, 18, 10), 3));

    d->insert(new TButton(TRect(13, 11, 23, 13), "~Z~apisz", cmOK, bfDefault));
    d->insert(new TButton(TRect(24, 11, 34, 13), "~A~nuluj", cmCancel, bfNormal));

    d->setData(&nowyTowarDialog);

    ushort wynik = deskTop->execView(d);
    destroy(d);

    if (wynik != cmCancel) {
        d->getData(&nowyTowarDialog);
        
        std::string sql("INSERT INTO magazyn(nazwa, cena, ilosc, data) VALUES(?, ?, ?, ?);");
        
        sqlite3_stmt *polecenie;
        sqlite3_prepare_v2(baza, sql.c_str(), sql.length(), &polecenie, nullptr);
        
        sqlite3_bind_text(polecenie, 1, nowyTowarDialog.nazwa, (std::string(nowyTowarDialog.nazwa)).length(), SQLITE_STATIC);
        
        double cena; sscanf(nowyTowarDialog.cena, "%lf", &cena);
        sqlite3_bind_double(polecenie, 2, cena);
        
        int ilosc = atoi(nowyTowarDialog.ilosc);
        sqlite3_bind_int(polecenie, 3, ilosc);
        
        std::string data(""); data += nowyTowarDialog.rok;
        data += "-"; data += nowyTowarDialog.miesiac;
        data += "-"; data += nowyTowarDialog.dzien;
        sqlite3_bind_text(polecenie, 4, data.c_str(), data.length(), SQLITE_STATIC);
        
        sqlite3_step(polecenie);
        sqlite3_finalize(polecenie);
    }
}

void TMagazynApp::edytujTowar()
{
    struct {
        char nr[11];
        char nazwa[128];
        char cena[11], ilosc[7];
        char rok[5], miesiac[3], dzien[3];
    } edytujTowarDialog;
    strcpy(edytujTowarDialog.nr, "");
    strcpy(edytujTowarDialog.nazwa, "");
    strcpy(edytujTowarDialog.ilosc, "");
    strcpy(edytujTowarDialog.cena , "");
    strcpy(edytujTowarDialog.dzien, "");
    strcpy(edytujTowarDialog.miesiac, "");
    strcpy(edytujTowarDialog.rok, "");

    TDialog *d = new TDialog(TRect(3, 3, 40, 19), "Edytuj towar");
    
    d->insert(new TStaticText(TRect(2, 2, 8, 3), "Nr:"));
    d->insert(new TInputLine(TRect(2, 3, 13, 4), 11));

    d->insert(new TStaticText(TRect(2, 4, 8, 5), "Nazwa:"));
    d->insert(new TInputLine(TRect(2, 5, 24, 6), 128));

    d->insert(new TStaticText(TRect(2, 6, 8, 7), "Cena:"));
    d->insert(new TInputLine(TRect(2, 7, 12, 8), 11));

    d->insert(new TStaticText(TRect(2, 8, 8, 9), "Ilość:"));
    d->insert(new TInputLine(TRect(2, 9, 9, 10), 7));

    d->insert(new TStaticText(TRect(2, 10, 8, 11), "Data:"));
    d->insert(new TInputLine(TRect(2, 11, 8, 12), 5));

    d->insert(new TStaticText(TRect(8, 11, 9, 12), "-"));
    d->insert(new TInputLine(TRect(9, 11, 13, 12), 3));

    d->insert(new TStaticText(TRect(13, 11, 14, 12), "-"));
    d->insert(new TInputLine(TRect(14, 11, 18, 12), 3));

    d->insert(new TButton(TRect(13, 13, 23, 15), "~Z~apisz", cmOK, bfDefault));
    d->insert(new TButton(TRect(24, 13, 34, 15), "~A~nuluj", cmCancel, bfNormal));

    d->setData(&edytujTowarDialog);

    ushort wynik = deskTop->execView(d);
    destroy(d);

    if (wynik != cmCancel) {
        d->getData(&edytujTowarDialog);
        
        std::string sql("REPLACE INTO magazyn(nazwa, cena, ilosc, data, rowid) VALUES(?, ?, ?, ?, ?);");
        
        sqlite3_stmt *polecenie;
        sqlite3_prepare_v2(baza, sql.c_str(), sql.length(), &polecenie, nullptr);
        
        sqlite3_bind_text(polecenie, 1, edytujTowarDialog.nazwa, (std::string(edytujTowarDialog.nazwa)).length(), SQLITE_STATIC);
        
        double cena; sscanf(edytujTowarDialog.cena, "%lf", &cena);
        sqlite3_bind_double(polecenie, 2, cena);
        
        int ilosc = atoi(edytujTowarDialog.ilosc);
        sqlite3_bind_int(polecenie, 3, ilosc);
        
        std::string data(""); data += edytujTowarDialog.rok;
        data += "-"; data += edytujTowarDialog.miesiac;
        data += "-"; data += edytujTowarDialog.dzien;
        sqlite3_bind_text(polecenie, 4, data.c_str(), data.length(), SQLITE_STATIC);
        
        int nr = atoi(edytujTowarDialog.nr);
        sqlite3_bind_int(polecenie, 5, nr);
        
        sqlite3_step(polecenie);
        sqlite3_finalize(polecenie);
    }
}
std::string double_na_string(double x) 
{
    std::ostringstream o;
    o << std::setprecision(2)
      << std::fixed
      << x;
    return o.str();
}

std::vector<std::string> generujLinie(std::string sql)
{
        
    sqlite3_stmt *polecenie;
    sqlite3_prepare_v2(baza, sql.c_str(), sql.length(), &polecenie, nullptr);
    
    std::vector<TTowar> towary;
    std::vector<std::string> linie;
    
    unsigned ileTowarow = 0;
    unsigned maxNr = 0, maxNazwa = 0, maxCena = 0, maxIlosc = 0, maxWartosc = 0;
    double lacznaWartosc = 0;
    
    while (sqlite3_step(polecenie) != SQLITE_DONE) {
        TTowar towar;
        
        towar.nr = std::to_string(sqlite3_column_int(polecenie, 0));
        
        towar.nazwa = std::string(reinterpret_cast<const char*>(sqlite3_column_text(polecenie, 1)));
        
        towar.cena = double_na_string(sqlite3_column_double(polecenie, 2));
        
        towar.ilosc = std::to_string(sqlite3_column_int(polecenie, 3));
        
        towar.data = std::string(reinterpret_cast<const char*>(sqlite3_column_text(polecenie, 4)));
        
        double wartosc = sqlite3_column_double(polecenie, 5);
        towar.wartosc = double_na_string(wartosc);;
        
        lacznaWartosc += wartosc;
        ++ileTowarow;
        
        towary.push_back(towar);
        
        if (maxNr < towar.nr.length()) maxNr = towar.nr.length();
        if (maxNazwa < towar.nazwa.length()) maxNazwa = towar.nazwa.length();
        if (maxCena < towar.cena.length()) maxCena = towar.cena.length();
        if (maxIlosc < towar.ilosc.length()) maxIlosc = towar.ilosc.length();
        if (maxWartosc < towar.wartosc.length()) maxWartosc = towar.wartosc.length();
    }
    /*
       nr | nazwa    | cena   | ilosc   | wartosc |data      
       ---------------------------------------------
       1  | drops    | 3.20   | 3       | 9.60    | 2024-10-23
    */
    
    std::string naglowek("");
    
    naglowek += "nr";
    if (maxNr > 2) for (int i = 0; i < maxNr - 2; ++i) naglowek += " ";
    else maxNr = 2;
    
    naglowek += " | nazwa";
    if (maxNazwa > 5) for (int i = 0; i < maxNazwa - 5; ++i) naglowek += " ";
    else maxNazwa = 5;
    
    naglowek += " | cena";
    if (maxCena > 4) for (int i = 0; i < maxCena - 4; ++i) naglowek += " ";
    else maxCena = 4;
    
    naglowek += " | ilość";
    if (maxIlosc > 5) for (int i = 0; i < maxIlosc - 5; ++i) naglowek += " ";
    else maxIlosc = 5;
    
    naglowek += " | wartość";
    if (maxWartosc > 7) for (int i = 0; i < maxWartosc - 7; ++i) naglowek += " ";
    else maxWartosc = 7;
    
    naglowek += " | data      ";
    
    std::string kreski("");
    for (int i = 0; i < naglowek.length(); ++i) kreski += "-";
    
    linie.push_back(naglowek); linie.push_back(kreski);
    
    for (const TTowar towar : towary) {
        std::string linia("");
        
        linia += towar.nr;
        if (towar.nr.length() < maxNr) for (int j = 0; j < maxNr - towar.nr.length(); ++j) linia += " ";
        linia += " | ";
        
        linia += towar.nazwa;
        if (towar.nazwa.length() < maxNazwa) for (int j = 0; j < maxNazwa - towar.nazwa.length(); ++j) linia += " ";
        linia += " | ";
        
        linia += towar.cena;
        if (towar.cena.length() < maxCena) for (int j = 0; j < maxCena - towar.cena.length(); ++j) linia += " ";
        linia += " | ";
        
        linia += towar.ilosc;
        if (towar.ilosc.length() < maxIlosc) for (int j = 0; j < maxIlosc - towar.ilosc.length(); ++j) linia += " ";
        linia += " | ";
        
        linia += towar.wartosc;
        if (towar.wartosc.length() < maxWartosc) for (int j = 0; j < maxWartosc - towar.wartosc.length(); ++j) linia += " ";
        linia += " | ";
        
        linia += towar.data;
        
        linie.push_back(linia);
    }
    linie.push_back("Ilość towarów: " + std::to_string(ileTowarow));
    linie.push_back("Łączna wartość: " + double_na_string(lacznaWartosc));
    return linie;
}

std::string TMagazynApp::dialogSortowania(bool przeterminowanie)
{
    struct {
        ushort cecha = 0, kolejnosc = 0;
    } opcje;
    TDialog *d = new TDialog(TRect(3, 3, 40, 16), "Sortowanie");
    
    d->insert(new TStaticText(TRect(2, 2, 14, 3), "sortuj wg.:"));
    d->insert(new TRadioButtons(TRect(2, 3, 14, 9),
    new TSItem("~N~umeru",
    new TSItem("N~a~zwy",
    new TSItem("~C~eny",
    new TSItem("~I~lości",
    new TSItem("~D~aty", 
    new TSItem("~W~artości", 0)))))) ) );
    
    d->insert(new TStaticText(TRect(15, 2, 29, 3), "jak sortować?"));
    d->insert(new TRadioButtons(TRect(15, 3, 29, 5),
    new TSItem("~R~osnąco",
    new TSItem("~M~alejąco", 0)) ) );
    
    d->insert(new TButton(TRect(13, 10, 23, 12), "~S~ortuj", cmOK, bfDefault));
    d->insert(new TButton(TRect(24, 10, 34, 12), "An~u~luj", cmCancel, bfNormal));
    
    d->setData(&opcje);
    
    ushort wynik = deskTop->execView(d);
    if (wynik == cmCancel) { destroy(d); return ""; }
    
    d->getData(&opcje);
    destroy(d);
    
    std::string sql("SELECT rowid, nazwa, cena, ilosc, data, cena * ilosc AS wartosc FROM magazyn ");
    if (przeterminowanie) sql += "WHERE data < date() ";
    sql += "ORDER BY ";
    sql += (const std::string[]){"rowid","nazwa","cena","ilosc","data","wartosc"}[opcje.cecha] + " ";
    sql += (const std::string[]){"ASC","DESC"}[opcje.kolejnosc] + ";";
    
    return sql;
}
void TMagazynApp::pokazMagazyn()
{
    std::string sql = dialogSortowania(false);
    if (sql != "") {
        std::vector<std::string> linie = generujLinie(sql);
        TRect r(1, 1, linie.at(1).length() + 4, 12);
        TOkno *okno = new TOkno(r, "Lista towarów", ++nrOkna, linie);
        deskTop->insert(okno);
    }
}

void TMagazynApp::przeterminowane()
{
    std::string sql = dialogSortowania(true);
    if (sql != "") {
        std::vector<std::string> linie = generujLinie(sql);
        TRect r(1, 1, linie.at(1).length() + 4, 12);
        TOkno *okno = new TOkno(r, "Towary przeterminowane", ++nrOknaPrzeterminowane, linie);
        deskTop->insert(okno);
    }
}

void TMagazynApp::usunTowar() {
    struct {
        char nr[11];
    } usunTowarDialog;
    strcpy(usunTowarDialog.nr, "");

    TDialog *d = new TDialog(TRect(3, 3, 40, 11), "Usuń towar");

    d->insert(new TStaticText(TRect(2, 2, 8, 3), "Numer:"));
    d->insert(new TInputLine(TRect(2, 3, 24, 4), 11));

    d->insert(new TButton(TRect(13, 5, 23, 7), " ~U~suń ", cmOK, bfDefault));
    d->insert(new TButton(TRect(24, 5, 34, 7), "~A~nuluj", cmCancel, bfNormal));

    d->setData(&usunTowarDialog);

    ushort wynik = deskTop->execView(d);
    destroy(d);

    if (wynik != cmCancel) {
        d->getData(&usunTowarDialog);
        
        std::string sql("DELETE FROM magazyn WHERE rowid = ?;");
        
        sqlite3_stmt *polecenie;
        sqlite3_prepare_v2(baza, sql.c_str(), sql.length(), &polecenie, nullptr);
        
        sqlite3_bind_int(polecenie, 1, atoi(usunTowarDialog.nr));
        
        sqlite3_step(polecenie);
        sqlite3_finalize(polecenie);
    }
}

void TMagazynApp::oProgramie()
{
    TDialog *d = new TDialog(TRect(4, 4, 55, 10), "O programie");
    d->insert(new TStaticText(TRect(2, 1, 32, 2), "..:: Magazyn wersja 2.1 ::.."));
    d->insert(new TStaticText(TRect(2, 2, 32, 3), "2024 Konrad Goliński"));
    d->insert(new TStaticText(TRect(2, 3, 36, 4), "Używa: Borland Turbo Vision 2.0"));
    d->insert(new TStaticText(TRect(2, 4, 50, 5), "Kocham informatykę (Miłosz i Mikołaj nie)"));
    deskTop->execView(d);
    destroy(d);
}

int main()
{
    bool czy_istnieje = std::filesystem::exists("magazyn.db");
    sqlite3_open("magazyn.db", &baza);
    if (baza == NULL) {
        std::cout << "BŁĄD: Nie można było otworzyć bazy danych. Sprawdź prawa dostępu do obecnej ścieżki.\n";
        return 1;
    }
    
    if (!czy_istnieje) {
        int wynik = sqlite3_exec(baza, "CREATE TABLE magazyn(nazwa TEXT NOT NULL, cena REAL NOT NULL, ilosc INT NOT NULL, data CHAR(10) NOT NULL);", NULL, 0, nullptr);
        if (wynik != SQLITE_OK) {
            std::cout << "BŁĄD: Nie można było utworzyć bazy danych. Sprawdź prawa dostępu do obecnej ścieżki.\n";
            return 1;
        }
    }
    
    TMagazynApp magazynApp;
    magazynApp.run();
    
    sqlite3_close(baza);
    return 0;
}
