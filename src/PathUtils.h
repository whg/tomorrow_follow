#pragma once

#include <vector>
#include "ofPolyline.h"
#include "ofxSvg.h"

struct FollowPath : public ofPolyline {
	float radius;
	FollowPath() : radius(5) {}
};

class FollowPathCollection {
public:

	FollowPathCollection() {}
	
	FollowPathCollection(const ofxSVG &svg, int resampleSpacing=5) {
		add(svg, resampleSpacing);
	}
    
    void add(const shared_ptr<FollowPath> &followPath) {
        mPaths.push_back(followPath);
    }
    
    void add(const std::vector<ofPath> &paths, float resampleSpacing=5) {
        for (auto path : paths) {
            path.setPolyWindingMode(OF_POLY_WINDING_ODD);
            const auto &outlines = path.getOutline();
            
            for (const auto &outline : outlines) {
                auto followPath = make_shared<FollowPath>();
                followPath->addVertices(outline.getResampledBySpacing(resampleSpacing).getVertices());
                add(followPath);
            }
        }

    }

	void add(const ofxSVG &svg, float resampleSpacing=5) {
        add(svg.getPaths(), resampleSpacing);
	}
    
	
	void resampleBySpacing(float spacing) {
		for (auto path : mPaths) {
			auto resampled = path->getResampledBySpacing(spacing);
			path->clear();
			path->addVertices(resampled.getVertices());
		}
	}
	
	
	ofRectangle getBoundingBox() const {
        ofRectangle bounds;
		
        for (size_t i = 0; i < mPaths.size(); ++i) {
            if (i == 0) {
                bounds = mPaths[i]->getBoundingBox();
            }
            else {
                bounds.growToInclude(mPaths[i]->getBoundingBox());
            }
        }
		
		return bounds;
	}
	
	float getTotalLength() const {
		float distance = 0;
		for (const auto path : mPaths) {
			distance+= path->getPerimeter();
		}
		return distance;
	}
	
	size_t getTotalVertices() const {
		size_t total = 0;
		for (const auto path : mPaths) {
			total+= path->size();
		}
		return total;
	}
	
	void centerPoints(ofVec2f p = ofVec2f(0)) {
		auto bounds = getBoundingBox();
		auto center = bounds.getCenter();
		
		for (auto path : mPaths) {
			for (auto &vert : path->getVertices()) {
				vert+= p - center;
			}
		}
	}
	
	
    void rotateY(float degrees) {
        ofVec3f axis(0, 1, 0);
        for (auto path : mPaths) {
            for (auto &vert : path->getVertices()) {
                vert.rotate(degrees, axis);
            }
        }
    }
	
	
    std::vector<shared_ptr<FollowPath>> mPaths;

};

