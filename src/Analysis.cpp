#include "Analysis.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>

using namespace std;

namespace Analysis {

std::vector<int> kmeans_cluster(const LandscapeMap& map, int k, int min_size)
{
    int w = map.width(), h = map.height();
    int n = w * h;
    vector<int> labels(n, -1);
    vector<pair<double,double>> points;
    vector<int> idx;
    for (int y=0;y<h;y++) for (int x=0;x<w;x++) if (map.at(x,y) >= 128) { points.emplace_back(x,y); idx.push_back(y*w+x); }
    int np = points.size(); if (np==0) return labels;
    k = min(k, np);
    vector<pair<double,double>> cent(k);
    mt19937 rng(42);
    uniform_int_distribution<int> ud(0,np-1);
    for (int i=0;i<k;i++) cent[i]=points[ud(rng)];
    bool changed=true; int it=0; while (changed && it++<100)
    {
        changed=false;
        vector<vector<int>> members(k);
        for (int i=0;i<np;i++){
            double best=1e18; int bi=0;
            for (int c=0;c<k;c++){
                double d=(points[i].first-cent[c].first)*(points[i].first-cent[c].first)+(points[i].second-cent[c].second)*(points[i].second-cent[c].second);
                if (d<best){best=d;bi=c;}
            }
            members[bi].push_back(i);
            int globalIdx=idx[i];
            if (labels[globalIdx]!=bi){labels[globalIdx]=bi;changed=true;}
        }
        for (int c=0;c<k;c++){
            if (members[c].size()>= (size_t)min_size){ double sx=0,sy=0; for (int i:members[c]){sx+=points[i].first; sy+=points[i].second;} cent[c]={sx/members[c].size(), sy/members[c].size()}; }
        }
    }
    std::ofstream kout("output/kmeans.txt");
    for (int i=0;i<np;i++){
        kout<<points[i].first<<" "<<points[i].second<<" "<<labels[idx[i]]<<"\n";
    }
    return labels;
}

std::vector<int> em_cluster(const LandscapeMap& map, int k, int min_size, int maxIter)
{
    (void)min_size;
    int w=map.width(), h=map.height();
    vector<pair<double,double>> points; vector<int> idx;
    for (int y=0;y<h;y++) for (int x=0;x<w;x++) if (map.at(x,y) >= 128) { points.emplace_back(x,y); idx.push_back(y*w+x); }
    int np = points.size(); vector<int> labels(w*h, -1); if (np==0) return labels;
    k = min(k, np);
    vector<pair<double,double>> mu(k);
    for (int i=0;i<k;i++) mu[i]=points[i%np];
    vector<double> weights(k,1.0/k);
    vector<double> varx(k,100.0), vary(k,100.0);
    vector<vector<double>> resp(np, vector<double>(k,0.0));
    constexpr double pi = 3.14159265358979323846;
    auto gaussian2d=[&](int c,const pair<double,double>& p){ double dx=p.first-mu[c].first; double dy=p.second-mu[c].second; double ex=(dx*dx/varx[c]+dy*dy/vary[c])/2.0; double denom=2*pi*sqrt(varx[c]*vary[c]); if (denom<=0) return 1e-12; return exp(-ex)/denom; };
    for (int iter=0;iter<maxIter;iter++){
        for (int i=0;i<np;i++){
            double s=0; for (int c=0;c<k;c++){ resp[i][c]=weights[c]*gaussian2d(c,points[i]); s+=resp[i][c]; }
            if (s<=0) s=1e-12; for (int c=0;c<k;c++) resp[i][c]/=s;
        }
        for (int c=0;c<k;c++){
            double Nk=0,mx=0,my=0; for (int i=0;i<np;i++){ Nk+=resp[i][c]; mx+=resp[i][c]*points[i].first; my+=resp[i][c]*points[i].second; }
            if (Nk<=1e-12) continue; mu[c].first=mx/Nk; mu[c].second=my/Nk; double vx=0,vy=0; for (int i=0;i<np;i++){ double dx=points[i].first-mu[c].first; double dy=points[i].second-mu[c].second; vx+=resp[i][c]*dx*dx; vy+=resp[i][c]*dy*dy; }
            varx[c]=max(1e-3, vx/Nk); vary[c]=max(1e-3, vy/Nk); weights[c]=Nk/np;
        }
    }
    // assign labels and write responsibilities
    ofstream fout("output/em_clusters.txt");
    ofstream fresp("output/em_responsibilities.txt");
    for (int i=0;i<np;i++){
        int best=0; double bv=resp[i][0]; for (int c=1;c<k;c++) if (resp[i][c]>bv){bv=resp[i][c];best=c;} labels[idx[i]]=best;
        fout<<points[i].first<<" "<<points[i].second<<" "<<best<<"\n";
        for (int c=0;c<k;c++){ fresp<<resp[i][c]; if (c+1<k) fresp<<" "; }
        fresp<<"\n";
    }
    fout.close(); fresp.close();
    return labels;
}

}
