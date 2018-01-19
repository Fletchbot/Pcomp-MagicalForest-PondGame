#include "WaterfallGameSource.h"
#include "ofxGPIO.h"
//--------------------------------------------------------------

//--------------------------------------------------------------
// main setup
void WaterfallGameSource::setup(){
    // Give our source a decent name
    name = "Waterfall Game FBO Source";
    // Allocate our FBO source, decide how big it should be
    allocate(800, 480);
    //Pinout
    gpio17.setup("17");
    gpio17.export_gpio();
    gpio17.setdir_gpio("in");

    screenWidth = fbo->getWidth();
    screenHeight = fbo->getHeight();
    offset = screenWidth/4;
    waterFallAreaX = screenWidth/3;
    waterFallAreaY = screenHeight/3;
    fieldCentreX = screenWidth - offset;

    setupGenField();
    setupWaterfall();
    setupAtoms();
    gameReset();



}
void WaterfallGameSource::setName(string _name){
    name = _name;
}
void WaterfallGameSource::gameReset(){
    ofSetVerticalSync(true);
    ofSetCircleResolution(20);

    atomRed = false;
    isleRed = false;
    atomState = 0;
    atomStartTime = ofGetElapsedTimeMillis();
    shockSavedTime = ofGetElapsedTimeMillis();
    shockTotalTime = 3000;
    isleStartTime = ofGetElapsedTimeMillis();
    gameSavedTime = ofGetElapsedTimeMillis();
    gameTotalTime = 60000*3;
    caughtCount = 0;
    buttonHits = 0;
    endGame = false;

    setupIslandRings();
        //atom setup
        for (unsigned int i = 0; i < atoms.size(); i++){
            atomParticle* tmpAtom = atoms.at(i);

            tmpAtom->pos.x = ofRandom(waterFallAreaX,screenWidth);
            tmpAtom->pos.y = ofRandom(screenHeight);

            tmpAtom->vel.x = ofRandom(-3.9, 3.9);
            tmpAtom->vel.y = ofRandom(-3.9, 3.9);

            tmpAtom->isDead  = 0;            
        }
}
//---------------------------------------------------------------
// Setup functions for all game objects
void WaterfallGameSource::setupGenField(){
    // Initialize particles
    int particleAmount = 75;
    for( int i = 0; i < particleAmount; i++ )
    {
        Particle* tmpParticle = new Particle();

        tmpParticle->pos.set(ofRandom(waterFallAreaX,screenWidth), ofRandom(screenHeight) );

        float tmpAngle = ofRandom( PI * 2.0f );
        float magnitude = 20.0f; // pixels per second
        tmpParticle->vel.set( cosf(tmpAngle) * magnitude, sinf(tmpAngle) * magnitude );

        tmpParticle->spacePartitioningIndexX = 0;
        tmpParticle->spacePartitioningIndexY = 0;

        tmpParticle->myID = i;

        particles.push_back( tmpParticle );
    }

    // Initialize storage we will use to optimize particle-to-particle distance checks
    spacePartitioningResX = 20;
    spacePartitioningResY = 20;

    spacePartitioningGridWidth = screenWidth / (float)spacePartitioningResX;
    spacePartitioningGridHeight= screenHeight / (float)spacePartitioningResY;

    for( int y = 0; y < spacePartitioningResY; y++ )
    {
        spacePartitioningGrid.push_back( vector< vector< Particle* > >() );
        for( int x = 0; x < spacePartitioningResX; x++ )
        {
            spacePartitioningGrid.at(y).push_back( vector< Particle* >() );
        }
    }
    lastUpdateTime = ofGetElapsedTimef();
}
void WaterfallGameSource:: setupWaterfall(){
    int dropAmount = 50;
    for (unsigned int i = 0; i < dropAmount; i++){
        Drop* tmpDrop = new Drop();
        tmpDrop->pos.set(ofRandom(0,waterFallAreaX*2), ofRandom(waterFallAreaY,screenHeight - waterFallAreaY));
        tmpDrop->vel.set( fabs(tmpDrop->vel.x) * 3.0, ofRandom(-0.5,0.5) ); //make the particles all be going across;
        tmpDrop->frc   = ofPoint(0,0,0);
        tmpDrop->scale = ofRandom(0.5, 1);
        drops.push_back( tmpDrop );
    }

}
void WaterfallGameSource:: setupAtoms(){
    int atomAmount = 5;
    for (unsigned int i = 0; i < atomAmount; i++){
        atomParticle* tmpAtom = new atomParticle();

        //the unique val allows us to set properties slightly differently for each particle
        tmpAtom->uniqueVal = ofRandom(-10000, 10000);

        tmpAtom->frc   = ofPoint(0,0,0);
        tmpAtom->drag  = ofRandom(0.98, 1);

        tmpAtom->scale = ofRandom(0.5, 1.0);

        tmpAtom->phase = 0;
        tmpAtom->pSpeed = ofRandom(-25,25);

        atoms.push_back( tmpAtom );

    }
}
void WaterfallGameSource:: setupIslandRings(){
    ringPhase = 0;
    myMouse.x = fieldCentreX;
    myMouse.y = screenHeight/2;
    vel.x = ofRandom(-2, 2);
    vel.y = ofRandom(-2, 2);
    frc = ofPoint(0,0,0);
    drag  = 1;
}
//---------------------------------------------------------------
// Main Update
void WaterfallGameSource::update(){
    int gamePassedTime = ofGetElapsedTimeMillis() - gameSavedTime;
    updateGenField();
    updateWaterfall();
    //Game States -----------------------------------------
    // current game
    if(endGame == false){
        water = ofColor(120,220,230,50);
        updateAtoms();
        updateIslandRings();
        // end game timer if someone plays but doesn't complete
        if(caughtCount > 1 && gamePassedTime > gameTotalTime){
            gameSavedTime = ofGetElapsedTimeMillis();
            gameReset();
        }
        // game ended
    }else if (endGame == true){
        water = ofColor(0,200,255,100);
    }
    //Pinout/button hits
    gpio17.getval_gpio(state_button);
    if(state_button == "1"){
        buttonHits++;
        if(atomRed == true && isleRed == true && buttonHits >= 1 && buttonHits <= 5){
            atomState = 1;
        } else if (endGame == true){
            gameReset();
        }
    } else if(state_button == "0"){
        buttonHits = 0;
    }
    usleep(50000);
    //--------
}
//---------------------------------------------------------------
// Update functions for all game objects
void WaterfallGameSource::updateGenField(){
    currTime = ofGetElapsedTimef();
    timeDelta = currTime - lastUpdateTime;
    lastUpdateTime = currTime;


    // update particle positions
    for( unsigned int i = 0; i < particles.size(); i++ )
    {
        Particle* tmpParticle = particles.at(i);
        tmpParticle->pos += tmpParticle->vel * timeDelta;

        tmpParticle->pos.x = fmodf( tmpParticle->pos.x, screenWidth );

        if( tmpParticle->pos.x < waterFallAreaX ) tmpParticle->pos.x += screenWidth;

        tmpParticle->pos.y = fmodf( tmpParticle->pos.y, screenHeight );

        if( tmpParticle->pos.y < 0 ) tmpParticle->pos.y += screenHeight;
    }

    // clear the space partitioning lists
    for( int y = 0; y < spacePartitioningResY; y++ )
    {
        for( int x = 0; x < spacePartitioningResX; x++ )
        {
            spacePartitioningGrid.at(y).at(x).clear();
        }
    }

    // add particles into the space partitioning grid
    for( unsigned int i = 0; i < particles.size(); i++ )
    {
        Particle* tmpParticle = particles.at(i);

        int tmpIndexX = tmpParticle->pos.x / spacePartitioningGridWidth;
        int tmpIndexY = tmpParticle->pos.y / spacePartitioningGridHeight;

        if( tmpIndexX < 0 )  tmpIndexX = 0;
        if( tmpIndexX >= spacePartitioningResX ) tmpIndexX = spacePartitioningResX-1;

        if( tmpIndexY < 0 )  tmpIndexY = 0;
        if( tmpIndexY >= spacePartitioningResY ) tmpIndexY = spacePartitioningResY-1;

        tmpParticle->spacePartitioningIndexX = tmpIndexX;
        tmpParticle->spacePartitioningIndexY = tmpIndexY;

        spacePartitioningGrid.at(tmpIndexY).at(tmpIndexX).push_back( tmpParticle );
    }

    // Now we update the line mesh, to do this we check each particle against every other particle, if they are
    // within a certain distance we draw a line between them. As this quickly becoems a pretty insane amount
    // of checks, we use our space partitioning scheme to optimize it all a little bit.
    ofPushStyle();
    lineMesh.clear();
    lineMesh.setMode( OF_PRIMITIVE_LINES );
    ofFloatColor scratchColor;
    if(endGame == false)scratchColor.set( 0, 1, 0.9);
    else if (endGame == true)scratchColor.set(1, 1, 1);

    float lineConnectionMaxDistance = 90; //ofMap( cosf( 100 ) , -1.0f, 1.0f, 10.0f, 100.0f); //   ofGetMouseY() / 10.0f;
    float lineConnectionMaxDistanceSquared = lineConnectionMaxDistance * lineConnectionMaxDistance;

    // how many slots do we need to check on each side?
    int spacePartitioningIndexDistanceX = ceil(lineConnectionMaxDistance / spacePartitioningGridWidth);
    int spacePartitioningIndexDistanceY = ceil(lineConnectionMaxDistance / spacePartitioningGridHeight);

    for( unsigned int particleIndex = 0; particleIndex < particles.size(); particleIndex++ )
    {
        Particle* tmpParticle = particles.at(particleIndex);

        // the particle knows where it is in the space partitioning grid, figure out which indices to loop between based
        // on how many slots the maximum line distance  can cover, then do a bounds check.
        int startIndexX = tmpParticle->spacePartitioningIndexX - spacePartitioningIndexDistanceX;
        if( startIndexX < 0 ) { startIndexX = 0; } if( startIndexX >= spacePartitioningResX ) { startIndexX = spacePartitioningResX-1;}

        int endIndexX   = tmpParticle->spacePartitioningIndexX + spacePartitioningIndexDistanceX;
        if( endIndexX < 0 ) { endIndexX = 0; } if( endIndexX >= spacePartitioningResX ) { endIndexX = spacePartitioningResX-1;}

        int startIndexY = tmpParticle->spacePartitioningIndexY - spacePartitioningIndexDistanceY;
        if( startIndexY < 0 ) { startIndexY = 0; } if( startIndexY >= spacePartitioningResY ) { startIndexY = spacePartitioningResY-1;}

        int endIndexY   = tmpParticle->spacePartitioningIndexY + spacePartitioningIndexDistanceY;
        if( endIndexY < 0 ) { endIndexY = 0; } if( endIndexY >= spacePartitioningResY ) { endIndexY = spacePartitioningResY-1;}

        for( int y = startIndexY; y < endIndexY; y++ )
        {
            for( int x = startIndexX; x < endIndexX; x++ )
            {
                for( unsigned int i = 0; i < spacePartitioningGrid.at(y).at(x).size(); i++ )
                {
                    Particle* tmpOtherParticle = spacePartitioningGrid.at(y).at(x).at(i);
                    if( tmpParticle->myID != tmpOtherParticle->myID )
                    {
                        ofVec2f diff = tmpParticle->pos - tmpOtherParticle->pos;
                        if( diff.lengthSquared() < lineConnectionMaxDistanceSquared )
                        {
                            scratchColor.a =  1.0f - (diff.length() / lineConnectionMaxDistance);

                            lineMesh.addVertex( tmpParticle->pos );
                            lineMesh.addColor( scratchColor );

                            lineMesh.addVertex( tmpOtherParticle->pos );
                            lineMesh.addColor( scratchColor );

                            lineMesh.addIndex( lineMesh.getNumVertices() - 2 );
                            lineMesh.addIndex( lineMesh.getNumVertices() - 1 );
                            ofPopStyle();
                        }
                    }
                }
            }
        }
    }
}
void WaterfallGameSource::updateWaterfall(){

    for (unsigned int i = 0; i < drops.size(); i++){

        Drop* tmpDrop = drops.at(i);

        tmpDrop->windY = ofSignedNoise(tmpDrop->pos.y * 0.003, tmpDrop->pos.x * 0.006, ofGetElapsedTimef() * 0.3);
        tmpDrop->frc.y = tmpDrop->windY * 0.01 + ofSignedNoise(tmpDrop->uniqueVal, tmpDrop->pos.x * 0.02) * 0.3;
        tmpDrop->frc.x = ofSignedNoise(tmpDrop->uniqueVal, tmpDrop->pos.y * 0.006, ofGetElapsedTimef()*0.2) * 0.09 + 0.18;
        tmpDrop->vel *= tmpDrop->drag;
        tmpDrop->vel += tmpDrop->frc * 0.4;
        tmpDrop->drag  = ofRandom(0.99, 1.05);
        tmpDrop->uniqueVal = ofRandom(-10000, 10000);

        //we do this so as to skip the bounds check for the bottom and make the particles go back across the screen
        if( tmpDrop->pos.x + tmpDrop->vel.x > waterFallAreaX + 20){
            tmpDrop->pos.y = ofRandom(waterFallAreaY, screenHeight - waterFallAreaY);
            tmpDrop->pos.x -= waterFallAreaX +20;
            tmpDrop->lifespan = 100;
        }
        //2 - UPDATE OUR POSITION

        tmpDrop->pos += tmpDrop->vel;
        //3 - LIMIT THE PARTICLES TO STAY ON SCREEN

        if( tmpDrop->pos.y > screenHeight - waterFallAreaY && tmpDrop->pos.x < waterFallAreaX ){
            tmpDrop->pos.y = screenHeight - waterFallAreaY;
            tmpDrop->vel.y*= -1.0;
        }
        else if( tmpDrop->pos.y < waterFallAreaY && tmpDrop->pos.x < waterFallAreaX){
            tmpDrop->pos.y = waterFallAreaY;
            tmpDrop->vel.y*= -1.0;
        }
        if( tmpDrop->pos.y > screenHeight  && tmpDrop->pos.x < waterFallAreaX*2 && tmpDrop->pos.x > waterFallAreaX){
            tmpDrop->pos.y = screenHeight;
            tmpDrop->vel.y*= -1.0;
        }
        else if( tmpDrop->pos.y < 0 && tmpDrop->pos.x < waterFallAreaX*2 && tmpDrop->pos.x > waterFallAreaX){
            tmpDrop->pos.y = 0;
            tmpDrop->vel.y*= -1.0;
        }
        if (tmpDrop->pos.x >offset && tmpDrop->pos.x< waterFallAreaX - 100){
            tmpDrop->drag = ofRandom(0.91,0.93);
            tmpDrop->lifespan -=7;

        }else if (tmpDrop->pos.x > waterFallAreaX - 100 && tmpDrop->pos.x < waterFallAreaX+20 ){
            tmpDrop->frc.y = tmpDrop->windY * 0.04 + ofSignedNoise(tmpDrop->uniqueVal, tmpDrop->pos.x * 0.02) * 0.6;
            tmpDrop->vel += tmpDrop->frc * 0.45;
            tmpDrop->drag = ofRandom(0.94,0.96);
            tmpDrop->lifespan -=8;
        }
    }
}
void WaterfallGameSource:: updateAtoms(){

    for (unsigned int i = 0; i < atoms.size(); i++){
        atomParticle* tmpAtom = atoms.at(i);
        tmpAtom->phase += tmpAtom->pSpeed;
        if( atomState == 0 ){

            ofPoint attractPt(myMouse.x, myMouse.y);
            tmpAtom->frc = attractPt-tmpAtom->pos;

            //let get the distance and only repel points close to the mouse
            float dist = tmpAtom->frc.length();
            tmpAtom->frc.normalize();

            tmpAtom->vel *= tmpAtom->drag;

            if( dist < 150 ){
                tmpAtom->vel += -tmpAtom->frc * 0.8; //notice the frc is negative
            }else{
                //if the particles are not close to us, lets add a little bit of random movement using noise. this is where uniqueVal comes in handy.
                tmpAtom->frc.x = ofSignedNoise(tmpAtom->uniqueVal, tmpAtom->pos.y * 0.1, ofGetElapsedTimef()*0.2);
                tmpAtom->frc.y = ofSignedNoise(tmpAtom->uniqueVal, tmpAtom->pos.x * 0.1, ofGetElapsedTimef()*0.2);
                tmpAtom->vel += tmpAtom->frc * 0.1;
            }
        } else if(atomState == 1){

            tmpAtom->vel.x = 0;
            tmpAtom->vel.y = 0;
            ofPoint attractPt(myMouse.x, myMouse.y );
            tmpAtom->frc = attractPt-tmpAtom->pos; // we get the attraction force/vector by looking at the mouse pos relative to our pos
            //let get the distance and only repel points close to the mouse
            float dist = tmpAtom->frc.length();
            if(dist<200 && dist > 100)tmpAtom->vel += tmpAtom->frc * 0.05;
            else if(dist<100){
                tmpAtom->vel += tmpAtom->frc * 0.8;
                tmpAtom->pos.x = screenWidth*100;
                tmpAtom->isDead = 1;
                caughtCount++;
            }
            if(caughtCount == 10){
                endGame = true;
            }

            int shockPassedTime = ofGetElapsedTimeMillis() - shockSavedTime;

            if(shockPassedTime > shockTotalTime ){
                atomState = 0;
                shockSavedTime = ofGetElapsedTimeMillis();
            }
        }

        //2 - UPDATE OUR POSITION

        tmpAtom->pos += tmpAtom->vel;


        //3 - (optional) LIMIT THE PARTICLES TO STAY ON SCREEN
        //we could also pass in bounds to check - or alternatively do this at the WaterfallGameSource level
        if( tmpAtom->pos.x > screenWidth - tmpAtom->scale*20 && tmpAtom->isDead == 0){
            tmpAtom->pos.x = screenWidth - tmpAtom->scale*20;
            tmpAtom->vel.x *= -1;
        }else if( tmpAtom->pos.x < waterFallAreaX && tmpAtom->isDead == 0 ){
            tmpAtom->pos.x = waterFallAreaX;
            tmpAtom->vel.x *= -1;
        }
        if( tmpAtom->pos.y > screenHeight - tmpAtom->scale*20 && tmpAtom->isDead == 0){
            tmpAtom->pos.y = screenHeight - tmpAtom ->scale*20;
            tmpAtom->vel.y *= -1;
        }
        else if( tmpAtom->pos.y < tmpAtom->scale*20 && tmpAtom->isDead == 0){
            tmpAtom->pos.y = tmpAtom->scale*20;
            tmpAtom->vel.y *= -1;
        }
    }
}
void WaterfallGameSource:: updateIslandRings(){
    ringPhase+=3;

    vel *= drag;
    frc.x = ofSignedNoise(cos(myMouse.y * 0.1), ofGetElapsedTimef()*0.2);
    frc.y = ofSignedNoise(sin(myMouse.x * 0.1), ofGetElapsedTimef()*0.2);
    vel += frc*0.6;
    myMouse += vel;

    //3 - (optional) LIMIT THE PARTICLES TO STAY ON SCREEN
    //we could also pass in bounds to check - or alternatively do this at the WaterfallGameSource level
    if( myMouse.x > screenWidth - 36){
        myMouse.x = screenWidth - 36;
        vel.x *= -1;
    }else if( myMouse.x < waterFallAreaX + 36){
        myMouse.x = waterFallAreaX + 36;
        vel.x *= -1;
    }
    if( myMouse.y > screenHeight - 36){
        myMouse.y = screenHeight - 36;
        vel.y *= -1;
    }
    else if( myMouse.y < 36){
        myMouse.y = 36;
        vel.y *= -1;
    }
}
//---------------------------------------------------------------
// Main Draw
void WaterfallGameSource::draw(){
    ofClear(0); //clear the buffer
    //background
    ofPushStyle();
    ofSetColor(water);
    ofDrawRectangle(0, 0,screenWidth, screenHeight );
    ofPopStyle();
    // draw objects
    drawGenField();
    drawWaterfall();
    drawAtoms(r1, r2);
    // FPS readout
   // ofSetColor(230);
   // string fpsStr = "frame rate: "+ofToString(ofGetFrameRate(), 2);
   // ofDrawBitmapString(fpsStr, 10,20);
    // draw objects when game is running
    if(endGame == false){
        drawIslandRings();
        // game ended restart msg
    } else if (endGame == true){
        ofPushMatrix();
        ofPushStyle();
        ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL);
        string replayStr = "Press Button To Replay";
        ofTranslate(fieldCentreX, screenHeight/2);
        ofRotate(-90);
        ofScale(1.5,1.5);
        ofDrawBitmapStringHighlight(replayStr,-100,0,ofColor(255),ofColor(0));
        ofSetColor(255);
        ofPopStyle();
        ofPopMatrix();
    }
}
//---------------------------------------------------------------
// Draw functions for all game objects
void WaterfallGameSource::drawGenField(){
    ofPushMatrix();
    ofPushStyle();
    ofEnableAlphaBlending();
    lineMesh.draw();
    ofDisableAlphaBlending();
    ofPopStyle();
    ofPopMatrix();
}
void WaterfallGameSource::drawWaterfall(){

    for (unsigned int i = 0; i < drops.size(); i++){
        Drop* tmpDrop = drops.at(i);

        if(tmpDrop->pos.x > 0 && tmpDrop->pos.x < waterFallAreaX){
            ofPushStyle();
            if(endGame == false) ofSetColor(100,220,255,60);
            else if(endGame == true) ofSetColor(0,200,255,60);
            ofDrawLine(tmpDrop->pos.x - 50,tmpDrop->pos.y + 10,tmpDrop->pos.x + 50, tmpDrop->pos.y + 10);
            ofDrawLine(tmpDrop->pos.x - 50,tmpDrop->pos.y - 10,tmpDrop->pos.x + 50, tmpDrop->pos.y - 10);
            ofPopStyle();
        }
        ofPushStyle();
        float alpha = ofMap(tmpDrop->lifespan, 100,0,255,0);
        if(endGame == false) ofSetColor(0,255,200,alpha);
        else if(endGame == true)ofSetColor(255,255,255,alpha);
        ofDrawEllipse( tmpDrop->pos.x, tmpDrop->pos.y, tmpDrop->scale * 12, tmpDrop->scale * 12);
        ofPopStyle();

        ofPushStyle();
        if(endGame == false) ofSetColor(0,255,255,alpha);
        else if(endGame == true) ofSetColor(0,190,255,alpha);
        ofDrawEllipse( tmpDrop->pos.x, tmpDrop->pos.y, tmpDrop->scale * 10, tmpDrop->scale * 10);
        ofPopStyle();
    }
}
void WaterfallGameSource:: drawAtoms(ofColor r1, ofColor r2){
    int timer1 = ofRandom(500,1000);
    int timer2 = ofRandom(1500,2000);
    int timer3 = ofRandom(2500,3000);
    int timer4 = ofRandom(4000,4500);
    int timer5 = ofRandom(5000,6000);

    if(ofGetElapsedTimeMillis() - atomStartTime < timer1){
        r1 = ofColor(50, 250, 50);
        r2 = ofColor(200, 50, 255);
        atomRed = false;
    }else if(ofGetElapsedTimeMillis() - atomStartTime < timer2){
        r1 = ofColor(80, 150, 250);
        r2 = ofColor(50, 180, 100);
    }else if(ofGetElapsedTimeMillis() - atomStartTime < timer3){
        r1 = ofColor(130, 150, 220);
        r2 = ofColor(100, 255, 150);
    }else if(ofGetElapsedTimeMillis() - atomStartTime < timer4){
        atomRed = true;
        r1 = ofColor(255, 50, 50);
        r2 = ofColor(200, 255, 50);

    } else if(ofGetElapsedTimeMillis() - atomStartTime < timer5){

        atomStartTime = ofGetElapsedTimeMillis();
    }
    for (unsigned int i = 0; i < atoms.size(); i++){

        atomParticle* tmpAtom = atoms.at(i);
        atom(tmpAtom->pos.x, tmpAtom->pos.y, tmpAtom->scale,tmpAtom->phase, r1, r2);
    }
}
void WaterfallGameSource:: drawIslandRings(){
    int numOfCircles = 6;
    int ringSpacing = 6;
    float cycles = 180;
    float phaseSpacing = cycles / numOfCircles;

    if(ofGetElapsedTimeMillis() - isleStartTime < 1500){
        isleC1 = ofColor(80, 150, 150);
        isleC2 = ofColor(50, 180, 90);
        isleRed = false;
    }else if(ofGetElapsedTimeMillis() - isleStartTime < 2500){
        isleC1 = ofColor(130, 150, 120);
        isleC2 = ofColor(100, 205, 150);
    }else if(ofGetElapsedTimeMillis() - isleStartTime < 3500){
        isleC1 = ofColor(80, 150, 150);
        isleC2 = ofColor(50, 180, 90);
    }else if(ofGetElapsedTimeMillis() - isleStartTime < 6500){
        isleRed = true;
        isleC1 = ofColor(255, 50, 50);
        isleC2 = ofColor(255, 100, 100);

    } else if(ofGetElapsedTimeMillis() - isleStartTime < 7500){

        isleStartTime = ofGetElapsedTimeMillis();
    }

    for (int i = numOfCircles; i > 0; i--) {
        ofColor c = isleC1.getLerped(isleC2, ofMap(i, 0, numOfCircles*2, 0, 1));
        ring(myMouse.x, myMouse.y, i*ringSpacing, ringPhase + phaseSpacing * i, c);
    }
}
//--------------------------------------------------------------
// play pieces
void WaterfallGameSource:: atom(float posX, float posY, float r, float p, ofColor c1, ofColor c2) {
    int numOfRings = 2;
    float phaseDiff = 180 / numOfRings;

    for (int rings = 0; rings < numOfRings; rings++) {
        float oscillation = 100;
        p = p + rings * phaseDiff;
        float localPhaseY = p * ofMap(cos(oscillation ), -1, 1, 0.5, 2);
        float localPhaseX = p * ofMap(sin(oscillation ), -1, 1, 0.5, 2);

        ofPushStyle();
        ofNoFill();
        ofSetLineWidth(4);

        ofPushMatrix();
        ofTranslate(posX, posY);
        ofRotateX(localPhaseX);
        ofSetColor(c1);
        ofDrawCircle(0, 0, r* 20);
        ofSetColor(255);
        ofSetLineWidth(1);
        ofDrawCircle(0, 0, r* 20.5);
        ofPopMatrix();
        ofPopStyle();

        ofPushStyle();
        ofNoFill();
        ofSetLineWidth(4);

        ofPushMatrix();
        ofTranslate(posX, posY);
        ofRotateY(localPhaseY);
        ofSetColor(c2);
        ofDrawCircle(0, 0, r* 5);
        ofSetColor(0);
        ofFill();
        ofDrawCircle(0, 0, r* 3);
        ofPopMatrix();
        ofPopStyle();
    }

}
void WaterfallGameSource:: ring(float posX, float posY, float r, float p, ofColor color) {
    ofPushMatrix();
    ofPushStyle();
    ofSetCircleResolution(100);
    ofTranslate(posX, posY);
    ofSetColor(color);
    ofDrawCircle(0, 0, r);
    float radDiff = ofMap(sin(ofDegToRad(p)),-1, 1, 1, 6);
    ofSetColor(water);
    ofDrawCircle(0, 0, r - radDiff);
    ofPopMatrix();
    ofPopStyle();

}



