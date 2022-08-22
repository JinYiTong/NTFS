#define UNICODE
#include <stdio.h>
#include "Volume.h"
#include "time.h"
#include <locale.h>



int main() {
    setlocale(LC_ALL, "chs");
    clock_t start, finish;
    double  duration;
    start = clock();
    Volume volume = Volume('c');
    volume.initVolume();
    cmpStrStr cmp = cmpStrStr(1,1);
    vector<wstring> *vectors = new vector<wstring>;
    vector<CString> obj = volume.findFile("chrome.exe",cmp,vectors);
    for(vector<CString>::iterator cit = obj.begin();
        cit != obj.end(); ++cit){
        printf("%ws\n",cit->GetBuffer());
    }
    finish = clock();
    duration = (double)(finish - start) / CLOCKS_PER_SEC;
    printf( "\n%f seconds", duration );
    return 0;
}