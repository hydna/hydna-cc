#include <iostream>

#include "debughelper.h"

using namespace std;

void debugPrint(string c, unsigned int ch, string msg) {
    cout << "HydnaDebug: ";

    cout.width(10);
    cout.fill(' ');
    cout << c;
    cout << ": ";
    
    cout.width(8);
    cout.fill(' ');
    cout.setf(ios::right);
    cout << ch;
    cout << ": " << msg << endl;    
}

void show_binary(unsigned int u) { 
  int t; 
 
  for(t=128; t>0; t = t/2) 
    if(u & t) cout << "1 "; 
    else cout << "0 "; 
 
  cout << "\n"; 
}

