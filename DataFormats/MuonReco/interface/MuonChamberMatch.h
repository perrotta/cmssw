#ifndef MuonReco_MuonChamberMatch_h
#define MuonReco_MuonChamberMatch_h

#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/MuonReco/interface/MuonSegmentMatch.h"
#include "DataFormats/MuonReco/interface/MuonRPCHitMatch.h"
#include <vector>

namespace reco {
  class MuonChamberMatch {
  public:
    std::vector<reco::MuonSegmentMatch> segmentMatches;  // segments matching propagated track trajectory
    std::vector<reco::MuonSegmentMatch> gemMatches;      // segments matching propagated track trajectory
    std::vector<reco::MuonSegmentMatch> me0Matches;      // segments matching propagated track trajectory
    std::vector<reco::MuonSegmentMatch> truthMatches;    // SimHit projection matching propagated track trajectory
    std::vector<reco::MuonRPCHitMatch> rpcMatches;       // rpc hits matching propagated track trajectory
    float edgeX;     // distance to closest edge in X (negative - inside, positive - outside)
    float edgeY;     // distance to closest edge in Y (negative - inside, positive - outside)
    float x;         // X position of the track
    float y;         // Y position of the track
    float xErr2;     // propagation uncertainty in X
    float yErr2;     // propagation uncertainty in Y
    float dXdZ;      // dX/dZ of the track
    float dYdZ;      // dY/dZ of the track
    float dXdZErr2;  // propagation uncertainty in dX/dZ
    float dYdZErr2;  // propagation uncertainty in dY/dZ
    DetId id;        // chamber ID

    int nDigisInRange;  // # of DT/CSC digis in the chamber close-by to the propagated track

    int detector() const { return id.subdetId(); }
    int station() const;

    std::pair<float, float> getDistancePair(float edgeX, float edgeY, float xErr, float yErr) const;
    float dist() const {
      return getDistancePair(edgeX, edgeY, xErr2, yErr2).first;
    }  // distance to absolute closest edge
    float distErr2() const {
      return getDistancePair(edgeX, edgeY, xErr2, yErr2).second;
    }  // propagation uncertainty in above distance
  };
}  // namespace reco

#endif
