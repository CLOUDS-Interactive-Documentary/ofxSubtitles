//
//  ofxSubtitles.cpp
//  
//
//  Created by Andrew Bueno on 7/26/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//  @The Studio for Creative Inquiry

#include <iostream>
#include "ofxSubtitles.h"


/*----------------------------------------------------------------------------
 
                                ofxSubtitles 
 
 ---------------------------------------------------------------------------*/

ofxSubtitles::ofxSubtitles(){
    currentlyDisplayedSub = NULL;
    subsLoaded = false;
    subsJustification = TEXT_JUSTIFICATION_CENTER;
}

void ofxSubtitles::setup(string fontPath, int fontSize, int fps, ofxSubtitleJustification j){
    font.loadFont(fontPath, fontSize);
    setFramesPerSecond(fps);
    setJustification(j);
}

bool ofxSubtitles::setup(string subPath, string fontPath, int fontSize, int fps, ofxSubtitleJustification j){
    setup(fontPath, fontSize, fps, j);
    if (!load(subPath)) {
        return false;
    }
    
    return true;
}

ofxSubtitles::~ofxSubtitles(){

}


bool ofxSubtitles::load(string path){
    cout<<"trying to load subtitle path = "<<path<<endl;
    filepath = path;
	bool isSRTFile = ofToLower(ofFilePath::getFileExt(path)) == "srt";
    srtFile = ofBufferFromFile(path);
    subtitleList.clear();
    currentlyDisplayedSub = NULL;
    if(srtFile.size() == 0 || !isSRTFile){
        ofLogError("Invalid subtitle file path. Check to see if you misspelled the path, that the file does not exist, or that the file is not a .srt");
        subsLoaded = false;
        return false;
    }

    int lineNumber = 0;

    while(!srtFile.isLastLine()){
        
        //Assign the subtitle unit its index number
        string srtLine = srtFile.getNextLine();
        lineNumber++;
        
        ofxSubtitleUnit title;
        title.setIndex(ofToInt(srtLine));
        
        //Fill the new subtitle struct with its start and end times
        lineNumber++;
        srtLine = srtFile.getNextLine();
        vector<string> times = ofSplitString(srtLine, " ");

        if(times.size() < 3){
            ofLogError("ofxSubtitles") << "Error parsing time from line " << srtLine << " in file " << path << " on line " << lineNumber << " with index " << title.getIndex() << endl;
            break;
        }
        
        if (timecode.millisForTimecode(times[0]) != -1)
        {
            // format: HH:MM:SS:MIL --> HH:MM:SS:MIL
            title.setStartTime(timecode.millisForTimecode(times[0]));
            title.setEndTime(timecode.millisForTimecode(times[2]));
        }
        else if (timecode.millisForTimecode2(times[0]) != -1) {
            // format: HH:MM:SS.D --> HH:MM:SS.D
            title.setStartTime(timecode.millisForTimecode2(times[0]));
            title.setEndTime(timecode.millisForTimecode2(times[2]));
        }
        else {
            // format: frame --> frame
            title.setStartTime(timecode.millisForFrame(ofToInt(times[0])));
            title.setEndTime(timecode.millisForFrame(ofToInt(times[2])));
        }

        if(title.getStartTime() == -1 || title.getEndTime() == -1 || title.getStartTime() > title.getEndTime()){
            ofLogError("ofxSubtitles") << "Error parsing time from line " << srtLine << " in file " << path << " on line " << lineNumber << " with index " << title.getIndex() << endl;
            break;
        }
        
        srtLine = srtFile.getNextLine();
        lineNumber++;
        
        //Add each spoken line to the subtitle struct
        while(!srtLine.empty()){
            title.addTitle(srtLine);
            srtLine = srtFile.getNextLine();
            lineNumber++;
        }
        subtitleList.push_back(title);
    }
    cout << "subtitles loaded " << subtitleList.size() << endl;
    
    subsLoaded = subtitleList.size() > 0;
    return subsLoaded;
}

bool ofxSubtitles::save(){
    return save(filepath);
}

bool ofxSubtitles::save(string path){
	stringstream ss;
    for(int i = 0; i < subtitleList.size(); i++){
		ss << subtitleList[i];
    }
    ofBuffer saveBuffer;
	ss >> saveBuffer;
//    cout << "buffer!! " << ss.str() << endl;
	return ofBufferToFile(ofToDataPath(path), saveBuffer, false);
}

ofxSubtitleUnit* ofxSubtitles::addSubtitle(long startTime, long endTime, string titleLine1, string titleLine2){
    ofxSubtitleUnit title;
	title.setStartTime(startTime);
    title.setEndTime(endTime);
    title.addTitle(titleLine1);
    title.addTitle(titleLine2);
    subtitleList.push_back( title );
    return &subtitleList[subtitleList.size()-1];
}


void ofxSubtitles::setJustification(ofxSubtitleJustification j){
    subsJustification = j;
}

void ofxSubtitles::loadFont(string path, int fontsize){
    font.loadFont(path, fontsize);
    
    if(!font.isLoaded()){
        ofLogError("ofxSubtitles") << "Font " << path << " couldn't be loaded";
    }
}

void ofxSubtitles::setFramesPerSecond(float fps){
    timecode.setFPS(fps);
}
       
//will return true once the very first time after a new title has been st
bool ofxSubtitles::isTitleNew(){
    return newTitle;
}

string ofxSubtitles::getFilepath(){
    return filepath;
}

bool ofxSubtitles::setTimeInMilliseconds(long milliseconds){
    currentTime = milliseconds;
    
    int minInd = 0, maxInd = subtitleList.size() - 1;

    bool titleAlreadyShowing = currentlyDisplayedSub != NULL;
    //If there is no currently displayed subtitle or the milliseconds parameter falls 
    //outside the scope of the current subtitle, search the list
    
    if(!titleAlreadyShowing){
		for(int i = 0; i < subtitleList.size(); i++){
            if(subtitleList[i].getStartTime() < milliseconds && subtitleList[i].getEndTime() >= milliseconds){
                currentlyDisplayedSub = &subtitleList[i];
                break;
            }
        }
    }
    else{
        bool pastCurrentTime = (currentlyDisplayedSub->getStartTime() >= milliseconds );
        bool beforeCurrentTime = (currentlyDisplayedSub->getEndTime() <= milliseconds );
        
        if(pastCurrentTime || beforeCurrentTime){
            currentlyDisplayedSub = NULL; // the next frame another one will be searched
        }
    }
    
    newTitle = (currentlyDisplayedSub != NULL && !titleAlreadyShowing);
    return currentlyDisplayedSub != NULL;
}


int ofxSubtitles::getNumTitles(){
    return subtitleList.size();
}

long ofxSubtitles::getDurationInMillis(){
    if(subsLoaded){
        return subtitleList[subtitleList.size()-1].getEndTime();
    }
    return 0;
}

float ofxSubtitles::getDurationInSeconds(){
    return getDurationInMillis()/1000.0;
}

bool ofxSubtitles::setTimeInSeconds(float seconds){
    return setTimeInMilliseconds(seconds * 1000);
}

//PLEASE MAKE SURE that you have your frame rate set correctly in timeCode!
bool ofxSubtitles::setTimeInFrames(int frames){
    return setTimeInMilliseconds(timecode.millisForFrame(frames));
}

void ofxSubtitles::setFadeInterval(long milliseconds){
    fadeTime = milliseconds;
}

//A binary search algorithm for finding a subtitle that should be displaying during the
//specified time. While the datasets are probably small enough that an iterative search
//would do alright, every little bit helps!
ofxSubtitleUnit *ofxSubtitles::searchSubtitleList(int minIndex, int maxIndex, long elapsedTime){
    
    while(minIndex <= maxIndex){
            
        int midIndex = (maxIndex - minIndex)/2 + minIndex;
        
        if(subtitleList[midIndex].getEndTime() <= elapsedTime){
            
            minIndex = midIndex + 1;
        }
        else if(subtitleList[midIndex].getStartTime() >= elapsedTime){
            
            maxIndex = midIndex - 1;
        }
        else{

            return &(subtitleList[midIndex]);
        }
    }
    
    return NULL;
}

void ofxSubtitles::draw(ofPoint point){
    draw(point.x,point.y);
}

//Due to the nature of subtitles, this function is designed with the assumption that you
//want to draw text relative to the center of the screen, and will input center x-coordinates
void ofxSubtitles::draw(float x, float y){
    
    bool canDraw = font.isLoaded() && currentlyDisplayedSub != NULL;
	if(!canDraw){
        return;
    }
    vector<string>& subLines = currentlyDisplayedSub->getLines();

    //add 1/2 the line height in the case of only one line, which works for making
    //1 and 2 line subtitle films look much nicer
	float centerOneLineAddition = 0;
    if(subLines.size() == 1){
        centerOneLineAddition = font.getLineHeight()*.5;
    }
    
    for(int i = 0; i < subLines.size(); i++){
        textBounds = font.getStringBoundingBox(subLines[i], 0, 0);
	    if(subsJustification == TEXT_JUSTIFICATION_LEFT){
            font.drawString(subLines[i], x - textBounds.width, y + font.getLineHeight()*i + centerOneLineAddition);
        }
	    else if(subsJustification == TEXT_JUSTIFICATION_CENTER){
            font.drawString(subLines[i],
                            x - textBounds.width/2,
                            y + font.getLineHeight()*i + centerOneLineAddition);
//            cout << "drawing line at " << subLines[i] << " at position " << (x - textBounds.width/2) << " " << (y + font.getLineHeight()*i + centerOneLineAddition) << endl;
            
        }
    }
}

string ofxSubtitles::getCurrentLine1(){
    if(currentlyDisplayedSub != NULL && currentlyDisplayedSub->getLines().size() > 0){
        return currentlyDisplayedSub->getLines()[0];
    }
    return "";
}

string ofxSubtitles::getCurrentLine2(){
    if(currentlyDisplayedSub != NULL && currentlyDisplayedSub->getLines().size() > 1){
        return currentlyDisplayedSub->getLines()[1];
    }
    return "";
}

vector<ofxSubtitleUnit>& ofxSubtitles::getSubtitles(){
    return subtitleList;
}

