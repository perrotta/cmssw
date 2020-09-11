#include "DataFormats/MuonDetId/interface/DTChamberId.h"
#include <DataFormats/MuonDetId/interface/MuonSubdetId.h>
#include "DataFormats/MuonDetId/interface/CSCDetId.h"
#include "DataFormats/MuonDetId/interface/RPCDetId.h"
#include "DataFormats/MuonDetId/interface/GEMDetId.h"
#include "DataFormats/MuonDetId/interface/ME0DetId.h"
#include "DataFormats/MuonReco/interface/MuonChamberMatch.h"
#include <cmath>
using namespace reco;

int MuonChamberMatch::station() const {
  if (detector() == MuonSubdetId::DT) {  // DT
    DTChamberId segId(id.rawId());
    return segId.station();
  }
  if (detector() == MuonSubdetId::CSC) {  // CSC
    CSCDetId segId(id.rawId());
    return segId.station();
  }
  if (detector() == MuonSubdetId::RPC) {  //RPC
    RPCDetId segId(id.rawId());
    return segId.station();
  }
  if (detector() == MuonSubdetId::GEM) {  //GEM
    GEMDetId segId(id.rawId());
    return segId.station();
  }
  if (detector() == MuonSubdetId::ME0) {  //ME0
    ME0DetId segId(id.rawId());
    return segId.station();
  }
  return -1;  // is this appropriate? fix this
}

std::pair<float, float> MuonChamberMatch::getDistancePair(float edgeX, float edgeY, float xErr2, float yErr2) const {
  if (edgeX > 9E5 && edgeY > 9E5 && xErr2 > 9E5 && yErr2 > 9E5)  // there is no track
    return std::make_pair(999999, 999999);

  float distance = 999999;
  float error2 = 999999;

  if (edgeX < 0 && edgeY < 0) {
    if (edgeX < edgeY) {
      distance = edgeY;
      error2 = yErr2;
    } else {
      distance = edgeX;
      error2 = xErr2;
    }
  } else if (edgeX < 0 && edgeY > 0) {
    distance = edgeY;
    error2 = yErr2;
  } else if (edgeX > 0 && edgeY < 0) {
    distance = edgeX;
    error2 = xErr2;
  } else if (edgeX > 0 && edgeY > 0) {
    distance = edgeX * edgeX + edgeY * edgeY;
    error2 = distance > 0 ? (edgeX * edgeX * xErr2 + edgeY * edgeY * yErr2) / distance : 0.f;
    distance = std::sqrt(distance);
  }

  return std::make_pair(distance, error2);
}
