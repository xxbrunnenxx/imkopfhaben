#include <cstdio>
// Nachbau der Zaehllogik aus ssd1677_driver.cpp / DisplayTask.
constexpr int kMaxPartialRefreshesBeforeFlush = 8;
constexpr int kMaxPartialRefreshesHardCap = 60;
struct Panel {
    int partial_refresh_count_=0; bool base=true; bool active=true;
    bool CanPartialRefresh(int max) const { return active && base && partial_refresh_count_<max; }
    bool DeferredFlushPending() const { return active && base && partial_refresh_count_>=kMaxPartialRefreshesBeforeFlush; }
    // true = es hat geblitzt (Voll-Refresh)
    bool Partial(){ if(!CanPartialRefresh(kMaxPartialRefreshesHardCap)){ partial_refresh_count_=0; return true;} ++partial_refresh_count_; return false; }
    bool Idle(){ if(!DeferredFlushPending()) return false; partial_refresh_count_=0; return true; }
};
int main(){
    int rc=0;
    // 1) Scrollen ohne Pause: erster erzwungener Blitz erst beim Hard Cap.
    {Panel p; int first=-1; for(int i=1;i<=100;i++) if(p.Partial()&&first<0) first=i;
     printf("Dauerscrollen: erster erzwungener Blitz bei Partial %d (erwartet 61)\n",first);
     if(first!=61) rc=1;}
    // 2) Der gemeldete Fall: 8 Partials, dann weiter -- vorher blitzte es bei 9.
    {Panel p; bool blitz=false; for(int i=0;i<8;i++) blitz|=p.Partial();
     printf("8 Partials am Stueck: Blitz=%s (erwartet nein), Flush faellig=%s (erwartet ja)\n",
            blitz?"ja":"nein", p.DeferredFlushPending()?"ja":"nein");
     if(blitz||!p.DeferredFlushPending()) rc=1;}
    // 3) Leerlauf holt den Flush nach und setzt den Zaehler zurueck.
    {Panel p; for(int i=0;i<8;i++) p.Partial();
     bool f=p.Idle();
     printf("Leerlauf nach 8: Flush gefahren=%s (erwartet ja), Zaehler=%d (erwartet 0)\n",
            f?"ja":"nein", p.partial_refresh_count_);
     if(!f||p.partial_refresh_count_!=0) rc=1;}
    // 4) Leerlauf ohne faelligen Flush blitzt nicht (kein Blitzen im Ruhezustand).
    {Panel p; for(int i=0;i<3;i++) p.Partial();
     bool f=p.Idle();
     printf("Leerlauf nach 3: Flush gefahren=%s (erwartet nein)\n", f?"ja":"nein");
     if(f) rc=1;}
    // 5) Mit Pausen wird nie mitten in der Bewegung geblitzt.
    {Panel p; bool blitz=false; for(int runde=0;runde<20;runde++){ for(int i=0;i<8;i++) blitz|=p.Partial(); p.Idle(); }
     printf("20 Scrollrunden je 8 mit Pause: Blitz mitten drin=%s (erwartet nein)\n", blitz?"ja":"nein");
     if(blitz) rc=1;}
    printf(rc? "\nFEHLSCHLAG\n":"\nALLE GRUEN\n");
    return rc;
}
