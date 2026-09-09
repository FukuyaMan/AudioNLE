#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {
enum class Admission { GuaranteedRealtime, BestEffortRealtime, PreparedRequired, NativeBypass, UnsupportedConfiguration };
Admission classify(int source, int project, int activeMixedViews, int block) {
  if (source == project) return Admission::NativeBypass;
  if (block != 256 || activeMixedViews < 0) return Admission::UnsupportedConfiguration;
  const bool common = (source==44100&&project==48000)||(source==48000&&project==44100)||(source==44100&&project==96000)||(source==96000&&project==44100)||(source==48000&&project==96000)||(source==96000&&project==48000);
  if (!common) return Admission::UnsupportedConfiguration;
  if (activeMixedViews <= 4) return Admission::GuaranteedRealtime;
  const int preparedAt = (source==44100&&project==48000)||(source==44100&&project==96000)||(source==96000&&project==44100) ? 8 : 16;
  return activeMixedViews >= preparedAt ? Admission::PreparedRequired : Admission::BestEffortRealtime;
}
void require(bool b) { if (!b) throw std::runtime_error("policy"); }
}
int main() { try {
  for (const auto& r : std::array<std::array<int,2>,6>{{{{44100,48000}},{{48000,44100}},{{44100,96000}},{{96000,44100}},{{48000,96000}},{{96000,48000}}}}) require(classify(r[0],r[1],4,256)==Admission::GuaranteedRealtime);
  require(classify(44100,48000,7,256)==Admission::BestEffortRealtime && classify(44100,48000,8,256)==Admission::PreparedRequired);
  require(classify(48000,44100,15,256)==Admission::BestEffortRealtime && classify(48000,44100,16,256)==Admission::PreparedRequired);
  require(classify(96000,48000,15,256)==Admission::BestEffortRealtime && classify(96000,48000,16,256)==Admission::PreparedRequired);
  require(classify(48000,48000,99,256)==Admission::NativeBypass && classify(44100,48000,4,512)==Admission::UnsupportedConfiguration);
  std::cout << "prepared SRC policy PASS table=ADR0002 native-bypass=1 mixed=conservative\n"; return 0;
 } catch (...) { return 1; } }
