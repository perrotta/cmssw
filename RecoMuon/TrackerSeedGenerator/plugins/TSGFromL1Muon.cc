
/** \class TSGFromL1Muon
 * Description: 
 * EDPRoducer to generate L3MuonTracjectorySeed from L1MuonParticles
 * \author Marcin Konecki
*/

#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/L1Trigger/interface/L1MuonParticle.h"
#include "DataFormats/L1Trigger/interface/L1MuonParticleFwd.h"
#include "DataFormats/TrajectorySeed/interface/TrajectorySeedCollection.h"
#include "DataFormats/MuonSeed/interface/L3MuonTrajectorySeedCollection.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"

#include "RecoPixelVertexing/PixelTrackFitting/interface/PixelTrackFilter.h"
#include "RecoTracker/TkTrackingRegions/interface/OrderedHitsGeneratorFactory.h"
#include "RecoTracker/TkTrackingRegions/interface/OrderedHitsGenerator.h"
#include "RecoTracker/TkSeedGenerator/interface/SeedFromProtoTrack.h"
#include "RecoMuon/TrackerSeedGenerator/interface/L1MuonPixelTrackFitter.h"
#include "RecoMuon/TrackerSeedGenerator/interface/L1MuonSeedsMerger.h"
#include "RecoMuon/TrackerSeedGenerator/interface/L1MuonRegionProducer.h"

#include <vector>


class TSGFromL1Muon : public edm::stream::EDProducer<> {
public:
  TSGFromL1Muon(const edm::ParameterSet& cfg);
  ~TSGFromL1Muon() override;
  void produce(edm::Event& ev, const edm::EventSetup& es) override;

private:
  edm::ParameterSet theConfig;
  edm::InputTag theSourceTag;
  edm::EDGetTokenT<l1extra::L1MuonParticleCollection> theSourceToken;
  edm::EDGetTokenT<PixelTrackFilter> theFilterToken;

  std::unique_ptr<L1MuonRegionProducer> theRegionProducer;
  std::unique_ptr<OrderedHitsGenerator> theHitGenerator;
  std::unique_ptr<L1MuonPixelTrackFitter> theFitter;
  std::unique_ptr<L1MuonSeedsMerger> theMerger;
};


using namespace reco;
using namespace l1extra;

namespace {
  template <class T>
  T sqr(T t) {
    return t * t;
  }
}  // namespace

TSGFromL1Muon::TSGFromL1Muon(const edm::ParameterSet& cfg) {
  produces<L3MuonTrajectorySeedCollection>();
  theSourceTag = cfg.getParameter<edm::InputTag>("L1MuonLabel");

  edm::ConsumesCollector iC = consumesCollector();
  theFilterToken = consumes<PixelTrackFilter>(cfg.getParameter<edm::InputTag>("Filter"));

  edm::ParameterSet hitsfactoryPSet = cfg.getParameter<edm::ParameterSet>("OrderedHitsFactoryPSet");
  std::string hitsfactoryName = hitsfactoryPSet.getParameter<std::string>("ComponentName");
  theHitGenerator = OrderedHitsGeneratorFactory::get()->create(hitsfactoryName, hitsfactoryPSet, iC);

  theSourceToken = iC.consumes<L1MuonParticleCollection>(theSourceTag);

  theRegionProducer = std::make_unique<L1MuonRegionProducer>(cfg.getParameter<edm::ParameterSet>("RegionFactoryPSet"));
  theFitter = std::make_unique<L1MuonPixelTrackFitter>(cfg.getParameter<edm::ParameterSet>("FitterPSet"));

  edm::ParameterSet cleanerPSet = theConfig.getParameter<edm::ParameterSet>("CleanerPSet");
  theMerger = std::make_unique<L1MuonSeedsMerger>(cleanerPSet);
}

TSGFromL1Muon::~TSGFromL1Muon() = default;

void TSGFromL1Muon::produce(edm::Event& ev, const edm::EventSetup& es) {
  auto result = std::make_unique<L3MuonTrajectorySeedCollection>();

  edm::Handle<L1MuonParticleCollection> l1muon;
  ev.getByToken(theSourceToken, l1muon);

  edm::Handle<PixelTrackFilter> hfilter;
  ev.getByToken(theFilterToken, hfilter);
  const PixelTrackFilter& filter = *hfilter;

  LogDebug("TSGFromL1Muon") << l1muon->size() << " l1 muons to seed from.";

  L1MuonParticleCollection::const_iterator muItr = l1muon->begin();
  L1MuonParticleCollection::const_iterator muEnd = l1muon->end();
  for (size_t iL1 = 0; muItr < muEnd; ++muItr, ++iL1) {
    if (muItr->gmtMuonCand().empty())
      continue;

    const L1MuGMTCand& muon = muItr->gmtMuonCand();
    l1extra::L1MuonParticleRef l1Ref(l1muon, iL1);

    theRegionProducer->setL1Constraint(muon);
    theFitter->setL1Constraint(muon);

    typedef std::vector<std::unique_ptr<TrackingRegion> > Regions;
    Regions regions = theRegionProducer->regions();
    for (Regions::const_iterator ir = regions.begin(); ir != regions.end(); ++ir) {
      L1MuonSeedsMerger::TracksAndHits tracks;
      const TrackingRegion& region = **ir;
      const OrderedSeedingHits& candidates = theHitGenerator->run(region, ev, es);

      unsigned int nSets = candidates.size();
      for (unsigned int ic = 0; ic < nSets; ic++) {
        const SeedingHitSet& hits = candidates[ic];
        std::vector<const TrackingRecHit*> trh;
        for (unsigned int i = 0, nHits = hits.size(); i < nHits; ++i)
          trh.push_back(hits[i]->hit());

        theFitter->setPxConstraint(hits);
        reco::Track* track = theFitter->run(es, trh, region);
        if (!track)
          continue;

        if (!filter(track, trh)) {
          delete track;
          continue;
        }
        tracks.push_back(L1MuonSeedsMerger::TrackAndHits(track, hits));
      }

      if (theMerger)
        theMerger->resolve(tracks);
      for (L1MuonSeedsMerger::TracksAndHits::const_iterator it = tracks.begin(); it != tracks.end(); ++it) {
        SeedFromProtoTrack seed(*(it->first), it->second, es);
        if (seed.isValid())
          (*result).push_back(L3MuonTrajectorySeed(seed.trajectorySeed(), l1Ref));

        //      GlobalError vtxerr( sqr(region->originRBound()), 0, sqr(region->originRBound()),
        //                                               0, 0, sqr(region->originZBound()));
        //      SeedFromConsecutiveHits seed( candidates[ic],region->origin(), vtxerr, es);
        //      if (seed.isValid()) (*result).push_back( seed.TrajSeed() );
        delete it->first;
      }
    }
  }

  LogDebug("TSGFromL1Muon") << result->size() << " seeds to the event.";
  ev.put(std::move(result));
}
