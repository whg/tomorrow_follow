
#pragma once

#include <memory>
#include "ofVec3f.h"
#include "ofParameter.h"

#include <thread>
#include <mutex>
#include <condition_variable>

#include "PathUtils.h"

struct AgentSettings {
	ofParameter<float> maxSpeed, maxForce;
	ofParameter<float> cohesionDistance, separationDistance;
	ofParameter<float> cohesionAmount, separationAmount;
	ofParameter<bool> moveAlongTargets;
};

struct FlockMeshSettings {
    enum FlockMeshType { LINES };
    union {
        float minLineDistance;
    };
};

struct Agent {
	ofVec3f mPos;
	AgentSettings &mSettings;
	
	ofVec3f mVel, mAcc;
	
	Agent(ofVec3f pos, AgentSettings &settings):
	mPos(pos),
	mSettings(settings),
	mVel(0),
	mAcc(0) {}
	
	void update() {
		
		mVel+= mAcc;
		mAcc.set(0);
		
		mVel.limit(mSettings.maxSpeed);
		
		mPos+= mVel;
	}
	
	ofVec3f seek(ofVec3f direction) {
		return (direction - mVel).limit(mSettings.maxForce);
	}
	
	ofVec3f seekPosition(ofVec3f target) {
		return seek(target - mPos);
	}
	
	void apply(ofVec3f force) {
		mAcc+= force;
	}
	
};

template<class AgentType>
class ofxFlock {
public:
	
	ofxFlock();

	virtual void addAgent(ofVec3f pos=ofVec3f(0));
	
	virtual void update();
	
	const std::vector<std::shared_ptr<AgentType>>& getAgents() const { return mAgents; }

	AgentSettings& getSettings() { return mAgentSettings; }
	
	ofParameter<bool> mDoFlock;
    
    void populateMesh(ofMesh &mesh, FlockMeshSettings settings);
	
protected:
	
	std::thread mCacheThread;
	std::mutex mCacheMutex;
	std::condition_variable mCaclCachesCondition;
//	std::atomic<bool> mCalcedCaches;
	
	void calcCaches();
	
protected:

	AgentSettings mAgentSettings;

	template <typename T>
	struct PingPongCache {
		std::vector<T> data[2];
		std::vector<int> counts[2];
		
		PingPongCache(): frontIndex(0), backIndex(1) {
			cout << "default constructed: " << frontIndex << endl;
		}
		
		void resize(size_t n) {
			for (size_t i = 0; i < 2; i++) {
				data[i].resize(n);
				counts[i].resize(n);
			}
		}
		
		void swap() {
			std::swap(frontIndex, backIndex);
		}
		
		std::vector<T>& getFrontData() { return data[frontIndex]; }
		std::vector<int>& getFrontCounts() { return counts[frontIndex]; }

		std::vector<T>& getBackData() { return data[backIndex]; }
		std::vector<int>& getBackCounts() { return counts[backIndex]; }
		
	protected:
		size_t frontIndex, backIndex;
	};

	std::vector<ofVec3f> mPositions;
	std::vector<std::shared_ptr<AgentType>> mAgents;
	
	PingPongCache<ofVec3f> mCohesionCache, mSeparationCache;
	
	size_t mNAgents;
		
	void wrapAgents();
};

inline ofVec3f getNormalPoint(ofVec3f p, ofVec3f a, ofVec3f b) {
	ofVec3f ap = p - a;
	ofVec3f ab = b - a;
	ab.normalize();
	ab *= ap.dot(ab);
	return a + ab;
}

struct FollowAgent : public Agent {
	std::shared_ptr<FollowPath> mPath;
	size_t mTargetIndex;
	
	FollowAgent(ofVec3f pos, AgentSettings &settings):
	Agent(pos, settings),
	mPath(nullptr),
	mTargetIndex(0) {}
	
	ofVec3f moveToNextTarget() {
	
		const auto &pathVertices = mPath->getVertices();
		const auto &target = pathVertices[mTargetIndex];
		
		if (mSettings.moveAlongTargets && target.squareDistance(mPos) < 9) {
			mTargetIndex = (mTargetIndex + 1) % pathVertices.size();
		}
		
		return seekPosition(target);
	
	}
	
	ofVec3f moveAlongPath() {
		
		auto nextPosition = mPos + mVel * 5;
		const auto &pathVerts = mPath->getVertices();
		const auto nVerts = pathVerts.size();
		
		float minDist = 1e8, dist;
		ofVec3f a, b, normalPoint, dir, target;
		
		for (size_t i = 0; i < nVerts; i++) {
			
			a = pathVerts[i];
			b = pathVerts[(i+1) % nVerts];
	
			normalPoint = getNormalPoint(nextPosition, a, b);
			
			if (normalPoint.x < min(a.x, b.x) ||
				normalPoint.x > max(a.x, b.x) ||
				normalPoint.y < min(a.y, b.y) ||
				normalPoint.y > max(a.y, b.y)) {
				
				normalPoint = a;
				
				a = b;
				b = pathVerts[(i+2) % nVerts];
				
			}
			
			dir = b - a;
			
			dist = nextPosition.squareDistance(normalPoint);
			
			if (dist < minDist) {
				
				minDist = dist;
				
				target = normalPoint + dir * 10;
			}
		}
		
		// if minDist > path.radius?
		
		return seekPosition(target);
	}
};

class ofxPathFollowingFlock : public ofxFlock<FollowAgent> {
public:
	
	enum FollowType { NONE, TARGET_FOLLOW, PATH_FOLLOW };
	
	ofxPathFollowingFlock();
	
	virtual void update() override;
	
	void addPathCollection(FollowPathCollection &&collection) {
		mPathCollections.push_back(std::move(collection));
	}
	
	void assignAgentsToCollection(int index=0, bool assignIndividual=false);
	
	
	ofParameter<float> mFollowAmount;
	ofParameter<int> mFollowType;
	
protected:
	std::vector<FollowPathCollection> mPathCollections;
	
};