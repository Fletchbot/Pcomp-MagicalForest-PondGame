#pragma once

#include "ofMain.h"
#include "FboSource.h"
#include "ofxGPIO.h"

class Particle
{
public:

    ofVec2f pos;
    ofVec2f vel;

    int myID;

    int spacePartitioningIndexX;
    int spacePartitioningIndexY;
};

class Drop{
public:
    ofPoint pos, frc;
    ofVec2f vel;
    float uniqueVal;
    int lifespan = 150;
    float drag;
    float windY;
    float scale;
};

class atomParticle{
public:
    atomParticle(){
        attractPoints = NULL;
    }

    void setAttractPoints( vector <ofPoint> * attract ){
        attractPoints = attract;
    }
    ofPoint pos;
    ofPoint vel;
    ofPoint frc;
    float isDead;


    float drag;
    float uniqueVal;
    float scale;
    float phase;
    float pSpeed;



    vector <ofPoint> * attractPoints;

};

class WaterfallGameSource : public ofx::piMapper::FboSource {
public:
    void setup();
    void update();
    void draw();
    void setName(string _name);
    void gameReset();
    void atom(float posX, float posY, float r, float p, ofColor c1, ofColor c2);
    void ring(float posX, float posY, float r, float p, ofColor color);

    void resetParticles();
    void setupGenField();
    void updateGenField();
    void drawGenField();


    void setupWaterfall();
    void updateWaterfall();
    void drawWaterfall();

    void setupAtoms();
    void updateAtoms();
    void drawAtoms(ofColor r1, ofColor r2);

    void setupIslandRings();
    void updateIslandRings();
    void drawIslandRings();

    float screenWidth;
    float screenHeight;
    int offset;
    int waterFallAreaX;
    int waterFallAreaY;
    int fieldCentreX;
    int caughtCount;
    int buttonHits;
    bool endGame;
    ofColor water;

    float gameSavedTime;
    float gameTotalTime;

    // GenField
    float currTime, timeDelta, lastUpdateTime;
    vector< Particle* > particles;

    vector< vector< vector< Particle* > > > spacePartitioningGrid;

    int					spacePartitioningResX;
    int					spacePartitioningResY;
    float				spacePartitioningGridWidth;
    float				spacePartitioningGridHeight;

    ofMesh				lineMesh;

    // Waterfall
    vector< Drop* > drops;

    //Atoms
    bool atomRed;
    bool isleRed;
    int atomState;

    vector<atomParticle* > atoms;

    vector<float> posX, posY;

    vector <ofPoint> attractPoints;
    vector <ofPoint> attractPointsWithMovement;

    float atomStartTime;

    float shockSavedTime;
    float shockTotalTime;

    ofColor r1, r2;

    //IslandRings
    float isleStartTime;
    float ringPhase;
    ofColor isleC1, isleC2;
    ofPoint myMouse;
    ofPoint vel;
    ofPoint frc;
    float drag;

    // Pinouts
    GPIO gpio17;
    string state_button;

};
