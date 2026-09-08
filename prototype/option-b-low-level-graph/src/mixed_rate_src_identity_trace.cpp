#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
void runWorkerCacheIdentityTrace();
namespace { using I=std::int64_t; constexpr I P=160,Q=147; I held(I t){return t*Q/P;} I boundary(I s){return(s*P+Q-1)/Q;} }
int main(){try { constexpr int n=512; std::array<float,n> in{},out{}; for(int i=0;i<n;++i)in[i]=float(i); juce::ZeroOrderHoldInterpolator z; z.process(double(Q)/P,in.data(),out.data(),n); int match=0,boundaryMiss=0,fractionalMiss=0; for(int t=0;t<=400;++t){auto actual=I(std::lround(out[t]));auto expected=held(t); if(actual==expected)++match;else {bool exact=(boundary(expected)==t); if(exact)++boundaryMiss;else ++fractionalMiss;} if(t>=157&&t<=163)std::cout<<"T="<<t<<" expected="<<expected<<" actual="<<actual<<"\n";} std::cout<<"IDENTITY total=401 matched="<<match<<" exact-miss="<<boundaryMiss<<" fractional-miss="<<fractionalMiss<<"\n"; if(fractionalMiss)throw std::runtime_error("fractional identity mismatch"); runWorkerCacheIdentityTrace(); return 0;}catch(const std::exception&e){std::cerr<<"OB-SRC-ID FAIL "<<e.what()<<'\n';return 1;}}
