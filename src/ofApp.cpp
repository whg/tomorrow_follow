#include "ofApp.h"


vector<ofVec3f> archimedianSphere(size_t samples, float radius, size_t numTurns=20) {
    
    int halfSamples = samples / 2;
    int skip = static_cast<int>(360.0f * numTurns / halfSamples);

    vector<ofVec3f> output;
    int j;
    float h, theta, fatness, x, y, z;
    
    for (int i = 0; i < halfSamples; i++) {
        j = i * skip;
        h = i / static_cast<float>(halfSamples / 2) - 1;
        theta = acos(h);
        fatness = sin(theta);
        y = cos(j * DEG_TO_RAD) * radius * fatness;
        x = h * radius;
        z = sin(j * DEG_TO_RAD) * radius * fatness;
        
        output.push_back(ofVec3f(x, y, z));
    }
    
    for (int i = halfSamples; --i > 0;) {
        j = i * skip;
        h = i / static_cast<float>(halfSamples / 2) - 1;
        theta = acos(h);
        fatness = sin(theta);
        y = cos(j * DEG_TO_RAD + PI) * radius * fatness;
        x = h * radius;
        z = sin(j * DEG_TO_RAD + PI) * radius * fatness;
        
        output.push_back(ofVec3f(x, y, z));
    }
    
    return output;
}

void ofApp::setup(){


    ofXml xml("base_settings.xml");
    xml.setTo("base");
    const int numAgents = xml.getIntValue("numAgents");
    const string colourPaletteFilename = xml.getValue("colourPalette");
    const bool paletteRandom = xml.getBoolValue("paletteRandom");
    const int windowWidth = xml.getIntValue("windowWidth");
    const int windowHeight = xml.getIntValue("windowHeight");
    const string fontName = xml.getValue("fontName");
    const int fontSize = xml.getIntValue("fontSize");
    const bool production = xml.getBoolValue("production");
    
    ofSetWindowShape(windowWidth, windowHeight);
    ofSetVerticalSync(false);
    ofSetFrameRate(60);

    fbo.allocate(ofGetWidth(), ofGetHeight(), GL_RGB32F);


    thing.load("dot.png");
	
    fbo.begin();
    ofClear(0,0,0,0);
    fbo.end();
    
	svg.load("TMRW logo hexagon black.svg");

    mFont.load(fontName, fontSize, true, true, true);
    
    mAgentMesh.setMode(OF_PRIMITIVE_LINES);

    ofImage colourPalette;
    colourPalette.load(colourPaletteFilename);
    unsigned char *colourPalettePixels = colourPalette.getPixels().getData();
    const int numPixels = colourPalette.getWidth() * colourPalette.getHeight();
    
    for (int i = 0; i < numAgents; i++) {
//		flock.addAgent(ofVec3f(ofRandom(-ofGetWidth(), ofGetWidth()), ofRandom(-ofGetWidth(), ofGetHeight()), ofRandom(-ofGetWidth(), ofGetHeight())));

        flock.addAgent(ofVec3f(0));

        unsigned char *col;
        if (paletteRandom) {
           col = &colourPalettePixels[static_cast<size_t>(ofRandom(numPixels)) * 3];
        }
        else {
            col = &colourPalettePixels[(i % numPixels) * 3];
        }
        ofFloatColor fcol = ofFloatColor(col[0]/255.0f, col[1]/255.0f, col[2]/255.0f);
        mAgentMesh.addColor(fcol);
        mAgentMesh.addColor(fcol); // add two because we're feeding lines into the geometry shader
    }
	
    // Main Logo
	FollowPathCollection logo(svg);
    logo.centerPoints();
    flock.addPathCollection(std::move(logo));
    
    
    // Messages
    FollowPathCollection fontCollection;
    fontCollection.add(mFont.getStringAsPoints("Hello"));
    fontCollection.centerPoints();
    
    flock.addPathCollection(std::move(fontCollection));
    flock.addPathCollection(std::move(fontCollection)); // this for time...

    // IcoSphere
    ofIcoSpherePrimitive sphere(180, 1);
    const auto &verts = sphere.getMesh().getVertices();
    const auto &indices = sphere.getMesh().getIndices();

    vector<ofPath> paths;
    for (size_t i = 0; i < indices.size(); i+= 3) {
        ofPath path;
        path.moveTo(verts[indices[i]]);
        path.lineTo(verts[indices[i+1]]);
        path.lineTo(verts[indices[i+2]]);
        paths.push_back(path);
    }
    
    
    FollowPathCollection sphereCollection;
    sphereCollection.add(paths);
    flock.addPathCollection(std::move(sphereCollection));

    // Archimedian Spiral Sphere
    auto spiralVerts = archimedianSphere(flock.size(), 180);
    shared_ptr<FollowPath> spiralPath = make_shared<FollowPath>();
    spiralPath->addVertices(spiralVerts);
    FollowPathCollection spiralCollection;
    spiralCollection.add(spiralPath);
    flock.addPathCollection(std::move(spiralCollection));
	
	
	gui.setup("settings", "settings/settings.xml", 20, 300);
    gui.add(mCycleSettings.set("cycle settings", false));
	gui.add(flock.getSettings().maxSpeed);
	gui.add(flock.getSettings().maxForce);
	gui.add(flock.getSettings().cohesionDistance);
	gui.add(flock.getSettings().separationDistance);
	gui.add(flock.getSettings().cohesionAmount);
	gui.add(flock.getSettings().separationAmount);
	gui.add(flock.mDoFlock);
	gui.add(flock.mFollowAmount);
	gui.add(flock.mFollowType);
	gui.add(flock.getSettings().moveAlongTargets);
    gui.add(mAlpha.set("alpha", 10, 0, 70));
    gui.add(mImageSize.set("image size", 2, 1, 30));
    gui.add(mRotationSpeed.set("rotation speed", 15, 1, 50));
    gui.add(mCloseDistanceThreshold.set("distance threshold", 4, 1, 320));
    gui.add(mSecondsToWaitBeforeNext.set("wait seconds", 1, 0.5, 10));
    gui.add(mForceChangeSeconds.set("force change seconds", 90, 5, 90));
    
    mPathIndex.addListener(this, &ofApp::pathIndexChanged);
    gui.add(mPathIndex.set("path index", 0, 0, 4));
    gui.add(mAgentsArrived.set("agents arrived", false));

	p2lShader.load("shaders/line2image.vert", "shaders/line2image.frag", "shaders/line2image.geom");

    mDrawGui = true;
    populateSettings();
    mArrivedCounter = 0;
    mLastChangeTime = 0;
    
    if (production) {
        mDrawGui = false;
        ofSetFullscreen(true);
    }
}

void ofApp::exit() {
}

//--------------------------------------------------------------
void ofApp::update() {
    
	ofSetWindowTitle(ofToString(ofGetFrameRate(), 2));
    
    if (ofGetFrameNum() % 10 == 0) {
        bool temp = flock.getSettings().moveAlongTargets;
        flock.getSettings().moveAlongTargets = false;
        mAgentsArrived = flock.agentsAtDestination(mCloseDistanceThreshold);
        flock.getSettings().moveAlongTargets = temp;
        
    }
    
    if (mAgentsArrived) {
        ++mArrivedCounter;
    }
    
    if (mCycleSettings &&
        (mArrivedCounter / 60.0 > mSecondsToWaitBeforeNext ||
        (ofGetElapsedTimef() - mLastChangeTime) > mForceChangeSeconds)) {
        size_t nextIndex = static_cast<size_t>(ofRandom(mSettingNames.size()));
        auto &name = mSettingNames[nextIndex];
        mSettingsGroup.getBool(name).set(true);
        cout << "set to: " << name << endl;
        mArrivedCounter = 0;
        mLastChangeTime = ofGetElapsedTimef();
    }
    
    if (ofGetFrameNum() % 30 == 0) {
        getDisplayMessage();
    }
}

//--------------------------------------------------------------
void ofApp::draw(){

    const auto &agents = flock.getAgents();
    
    fbo.begin();
    
    ofSetColor(0, 0, 0, mAlpha);
    ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());

    mAgentMesh.clearVertices();

    float hue = 0.4;
    for (const auto agent : agents) {
        

        mAgentMesh.addVertex(agent->mPos);
        mAgentMesh.addVertex(agent->mPos + ofVec2f(mImageSize, 0));

        
    }
    
    flock.update();
    
    
    p2lShader.begin();
    p2lShader.setUniformTexture("tex", thing, 0);
    p2lShader.setUniform2f("texSize", thing.getWidth(), thing.getHeight());
    
    
    p2lShader.setUniformMatrix4f("transformMatrix", getTransformMatrix());
    
    mAgentMesh.draw(OF_MESH_FILL);

    
    p2lShader.end();


    fbo.end();

    fbo.draw(0, 0);


	if (mDrawGui) {
        gui.draw();
        mSettingsPanel.draw();
        
        ofSetColor(255);
        ofDrawBitmapString(ofToString(flock.getAgents().size()), 10, ofGetHeight() - 12);

    }
    
    
	
}


void ofApp::keyPressed(int key) {
    if (key == ' ') mDrawGui ^= true;
    if (key == 'f') ofToggleFullscreen();
    
    if (key == 's') {
        string filename = ofSystemTextBoxDialog("name the settings file:");
        gui.saveToFile("settings/" + filename + ".xml");
        
        populateSettings();
    }
}

void ofApp::pathIndexChanged(int &index) {
    
    if (index == 2) {
        FollowPathCollection fontCollection;
        auto timeStamp = ofGetTimestampString("%H:%M");
        fontCollection.add(mFont.getStringAsPoints(timeStamp));
        fontCollection.centerPoints();
        flock.setPathCollection(2, std::move(fontCollection));
    }

    flock.assignAgentsToCollection(index, true);
}

ofMatrix4x4 ofApp::getTransformMatrix() {
    ofMatrix4x4 matrix;
    
    float t = ofGetElapsedTimef() * mRotationSpeed;
    
    int r = static_cast<int>(t);
    float frac = t - r;
    r %= 360;
    float rot = r + frac;
    if (rot > 180) rot = 360 - rot;
    
    float s = ofGetElapsedTimef() * 0.05;
    float ss = sin(s) * 0.5 + 1.5;
    
    matrix.rotate(rot - 90, 0, 1, 0);
    matrix.scale(ss, ss, ss);
    matrix.translate(ofGetWidth()/2, ofGetHeight()/2, 0);
    
    return matrix;
}

void ofApp::populateSettings() {
    ofDirectory dir("settings");
    auto &files = dir.getFiles();
    
    mSettingsGroup.setName("...");
    mSettingsGroup.clear();
    
    for (auto file : files) {
        auto name = file.getBaseName();
        mSettingsGroup.addChoice(name);
        mSettingNames.push_back(name);
    }
    
    ofAddListener(mSettingsGroup.changeEvent, this, &ofApp::settingChanged);
    mSettingsPanel.setup("saved settings");
    mSettingsPanel.clear();
    mSettingsPanel.add(mSettingsGroup);
    
}

void ofApp::settingChanged(ofxRadioGroupEventArgs &args) {
    bool tempCycle = mCycleSettings;
    gui.loadFromFile("settings/" + args.name + ".xml");
    mCycleSettings = tempCycle;
}

void ofApp::getDisplayMessage() {
    
    ofFile file("message.txt");
    
    ofBuffer buffer = file.readToBuffer();
	auto text = buffer.getText();
    FollowPathCollection fontCollection;
    fontCollection.add(mFont.getStringAsPoints(text));
    fontCollection.centerPoints();
    flock.setPathCollection(1, std::move(fontCollection));
    
    // live reload
    if (mPathIndex == 1) {
        flock.assignAgentsToCollection(1, true);
    }
    
}

