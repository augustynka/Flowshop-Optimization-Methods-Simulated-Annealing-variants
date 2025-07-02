#include <bits/stdc++.h>
using namespace std;

// Helper: split string by delimiter
vector<string> split(const string &s, char delim) {
    vector<string> elems;
    string item;
    istringstream iss(s);
    while (getline(iss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        cerr << "Użycie: " << argv[0] << " plik1.csv [plik2.csv ...]" << endl;
        return 1;
    }

    for(int f=1; f<argc; ++f) {
        string in_name = argv[f];
        ifstream in(in_name);
        if(!in) {
            cerr << "Nie można otworzyć pliku: " << in_name << endl;
            continue;
        }

        string header;
        getline(in, header);
        auto cols = split(header, ',');
        int idxTyp=-1, idxN=-1, idxH=-1, idxTau=-1, idxPmax=-1, idxLmax=-1;
        for(int i=0;i<(int)cols.size();++i) {
            if(cols[i]=="TypTestu") idxTyp=i;
            if(cols[i]=="n")        idxN=i;
            if(cols[i]=="h")        idxH=i;
            if(cols[i]=="tau")      idxTau=i;
            if(cols[i]=="Pmax")     idxPmax=i;
            if(cols[i]=="Lmax")     idxLmax=i;
        }
        if(idxTyp<0||idxLmax<0) {
            cerr<<"Brak wymaganych kolumn w "<<in_name<<endl;
            in.close();
            continue;
        }

        struct Row {
            vector<string> fields;
            double avg;
        };
        vector<Row> rows;
        string line;
        while(getline(in,line)) {
            if(line.empty()) continue;
            auto flds = split(line, ',');
            if((int)flds.size() != (int)cols.size()) continue;
            rows.push_back({flds,0.0});
        }
        in.close();

        map<string,pair<double,int>> stats;
        for(auto &r : rows) {
            string typ = r.fields[idxTyp];
            string val;
            if(typ=="n")      val = r.fields[idxN];
            else if(typ=="h") val = r.fields[idxH];
            else if(typ=="tau") val = r.fields[idxTau];
            else if(typ=="range") val = r.fields[idxPmax];
            else val = "";
            string key = typ + ":" + val;
            double l = stod(r.fields[idxLmax]);
            stats[key].first += l;
            stats[key].second += 1;
        }

        for(auto &r : rows) {
            string typ = r.fields[idxTyp];
            string val;
            if(typ=="n")      val = r.fields[idxN];
            else if(typ=="h") val = r.fields[idxH];
            else if(typ=="tau") val = r.fields[idxTau];
            else if(typ=="range") val = r.fields[idxPmax];
            string key = typ + ":" + val;
            auto &pr = stats[key];
            r.avg = pr.first / pr.second;
        }

        string out_name = in_name;
        auto pos = out_name.rfind(".csv");
        if(pos!=string::npos) out_name.insert(pos, "_with_avg");
        else out_name += "_with_avg.csv";

        ofstream out(out_name);
        out << header << ",średnia\n";
        for(auto &r : rows) {
            for(int i=0;i<(int)r.fields.size();++i) {
                if(i) out<<',';
                out<<r.fields[i];
            }
            out<<','<<fixed<<setprecision(2)<<r.avg<<'\n';
        }
        out.close();
        cout<<"Zapisano: "<<out_name<<endl;
    }

    return 0;
}
