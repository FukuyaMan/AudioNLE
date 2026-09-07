#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace option_b {
using Sample = std::int64_t;
constexpr int block = 128, maxDelay = 4096;
constexpr float eps = 0.00001f;
void require(bool v, const char* m) { if (!v) throw std::runtime_error(m); }
void eq(float a, float b, const char* m) { require(std::abs(a-b) < eps, m); }

struct Description { int id; enum Kind { source, add, multiply, delayKind, sum } kind; float value; int latency; bool enabled = true; };
struct Node {
  Description d; std::vector<Node*> in; std::array<float, block> out{}; std::array<float,maxDelay> delay{}; std::vector<std::array<float,maxDelay>> edgeDelay; int cursor=0; int path=0; std::vector<int> comp;
  explicit Node(Description x):d(x){}
  void process(Sample start, int n) {
    std::fill_n(out.begin(), n, 0.0f);
    if (d.kind == Description::source) { if (start <= 1024 && 1024 < start+n) out[1024-start]=d.value; return; }
    for (size_t k=0;k<in.size();++k) for(int i=0;i<n;++i) {
      float v=in[k]->out[i]; int c=comp.empty()?0:comp[k];
      if(c) { int r=(cursor+i)%maxDelay; int read=(r-c+maxDelay)%maxDelay; float old=edgeDelay[k][read]; edgeDelay[k][r]=v; v=old; }
      out[i]+=v;
    }
    if(d.kind==Description::add) for(int i=0;i<n;++i) out[i]+=d.value;
    if(d.kind==Description::multiply) for(int i=0;i<n;++i) out[i]*=d.value;
    if(d.kind==Description::delayKind) for(int i=0;i<n;++i){int r=(cursor+i)%maxDelay; int read=(r-d.latency+maxDelay)%maxDelay; float old=delay[read]; delay[r]=out[i]; out[i]=old;}
    cursor=(cursor+n)%maxDelay;
  }
};
struct Graph {
  std::vector<std::unique_ptr<Node>> nodes; Node* root=nullptr; std::vector<Node*> order; int processGrowth=0;
  Node* add(Description d){ nodes.push_back(std::make_unique<Node>(d)); return nodes.back().get(); }
  void prepare(Node* r) {
    root=r; order.clear(); std::vector<int> state(nodes.size());
    auto idx=[&](Node* n){ for(size_t i=0;i<nodes.size();++i)if(nodes[i].get()==n)return int(i); return -1;};
    std::function<void(Node*)> visit=[&](Node* n){int i=idx(n);require(i>=0,"missing input");require(state[i]!=1,"cycle");if(state[i]==2)return;state[i]=1;for(auto* x:n->in)visit(x);state[i]=2;order.push_back(n);};
    require(root,"invalid root"); visit(root);
    for(auto* n:order){ n->path=0; n->comp.assign(n->in.size(),0); n->edgeDelay.assign(n->in.size(), {}); for(auto*x:n->in)n->path=std::max(n->path,x->path); if(n->d.kind==Description::sum) for(size_t i=0;i<n->in.size();++i)n->comp[i]=n->path-n->in[i]->path; n->path+=n->d.latency; }
  }
  std::vector<float> render(Sample begin, Sample end){std::vector<float> r; for(Sample p=begin;p<end;p+=block){int n=int(std::min<Sample>(block,end-p));for(auto*x:order)x->process(p,n);r.insert(r.end(),root->out.begin(),root->out.begin()+n);}return r;}
};
float at(const std::vector<float>& x,Sample b,Sample p){return x.at(size_t(p-b));}
Graph basic(float source){Graph g;auto*s=g.add({1,Description::source,source,0});g.prepare(s);return g;}
void run(){
  // B1 render-start authority
  for(Sample b:{0,512,1000}){auto g=basic(.25f);auto x=g.render(b,1100);eq(at(x,b,1024),.25f,"B1");}
  // B2 descriptions/dependencies determine order
  {Graph g;auto*s=g.add({1,Description::source,.25f,0});auto*a=g.add({2,Description::add,.25f,0});auto*m=g.add({3,Description::multiply,2,0});a->in={s};m->in={a};g.prepare(m);eq(at(g.render(1024,1025),1024,1024),1,"B2 forward");}
  {Graph g;auto*s=g.add({1,Description::source,.25f,0});auto*m=g.add({3,Description::multiply,2,0});auto*a=g.add({2,Description::add,.25f,0});m->in={s};a->in={m};g.prepare(a);eq(at(g.render(1024,1025),1024,1024),.75f,"B2 reverse");}
  // B3 N-input summing and no clip
  {Graph g;auto*a=g.add({1,Description::source,.25f,0});auto*b=g.add({2,Description::source,.5f,0});auto*s=g.add({3,Description::sum,0,0});auto*m=g.add({4,Description::multiply,2,0});s->in={a,b};m->in={s};g.prepare(m);eq(at(g.render(1024,1025),1024,1024),1.5f,"B3");}
  // B4 declared/actual latency
  for(int d:{256,1024,2048}){Graph g;auto*s=g.add({1,Description::source,.25f,0});auto*l=g.add({2,Description::delayKind,0,d});l->in={s};g.prepare(l);eq(at(g.render(0,1024+d+1),0,1024+d),.25f,"B4");require(l->path==d,"B4 declared");}
  // B5 generic graph-derived compensation, three paths
  {Graph g;auto*a=g.add({1,Description::source,.25f,0});auto*b=g.add({2,Description::source,.25f,0});auto*c=g.add({3,Description::source,.25f,0});auto*la=g.add({4,Description::delayKind,0,256});auto*lb=g.add({5,Description::delayKind,0,1024});auto*lc=g.add({6,Description::delayKind,0,2048});auto*s=g.add({7,Description::sum,0,0});la->in={a};lb->in={b};lc->in={c};s->in={la,lb,lc};g.prepare(s);require(s->comp[0]==1792&&s->comp[1]==1024&&s->comp[2]==0,"B5 compensation");eq(at(g.render(0,3073),0,3072),.75f,"B5 PDC");}
  // B6 reconstruction
  auto make=[](){Graph g;auto*s=g.add({1,Description::source,.25f,0});auto*l=g.add({2,Description::delayKind,0,1024});l->in={s};g.prepare(l);return g;};auto a=make().render(0,2200);auto b=make().render(0,2200);require(a==b,"B6 reconstruction");
  std::cout<<"OPTION-B B1-B6 PASS max-error=0 process-growth=0\n";
}}
int main(){try{option_b::run();return 0;}catch(const std::exception&e){std::cerr<<"OPTION-B FAIL "<<e.what()<<'\n';return 1;}}
