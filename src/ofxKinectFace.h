//
//  ofxKinectFace
//
//  Created by flatscape
//
//  Released under the MIT license
//  http://opensource.org/licenses/mit-license.php
//
#pragma once

#include "ofMain.h"
#include <Kinect.h>
#include <Kinect.Face.h>

#pragma mark - KinectBase

// kinect face common class
class KinectBase
{
public:
	KinectBase();
	~KinectBase();

	bool setup();
	void update();
	virtual void draw(){};
	void drawColor(int x, int y);
	void drawColor(int x, int y, int w, int h);
	void close();
	ofQuaternion getRotation(int idx);
	bool getIsFaceValid(int idx);
	int getBodyCount();
	int getWidth();
	int getHeight();

protected:
	virtual void processFaces(){};
	bool updateBodyData(IBody** bodies);
	ColorSpacePoint cameraToScreen(CameraSpacePoint pp);

	template<class Interface>
	inline void SafeRelease(Interface *& pInterfaceToRelease)
	{
		if (pInterfaceToRelease != NULL)
		{
			pInterfaceToRelease->Release();
			pInterfaceToRelease = NULL;
		}
	}

	IKinectSensor* sensor;
	IColorFrameReader* colorFrameReader;
	IBodyFrameReader* bodyFrameReader;
	ICoordinateMapper* coordinateMapper;
	Vector4 faceRotation[BODY_COUNT];
	bool isFaceValid[BODY_COUNT];

	ofImage* color;
};

#pragma mark - ofxKinectFace

// kinect face wrapper
class ofxKinectFace : public KinectBase
{
public:
	ofxKinectFace();

	void setup();
	void update();
	void draw();
	ofRectangle getFaceRect(int idx);
	ofPoint getFacePoint(int idx, FacePointType type);
	DetectionResult getFaceProperty(int idx, FaceProperty type);

private:
	void processFaces();
    
    IFaceFrameSource* faceFrameSources[BODY_COUNT]; // Face sources
    IFaceFrameReader* faceFrameReaders[BODY_COUNT]; // Face readers
	ofRectangle faceRect[BODY_COUNT];
	PointF facePoints[BODY_COUNT][FacePointType::FacePointType_Count];
	DetectionResult faceProperties[BODY_COUNT][FaceProperty::FaceProperty_Count];
};

#pragma mark - ofxKinectHDFace

// kinect hd face wrapper
class ofxKinectHDFace : public KinectBase
{
public:
	ofxKinectHDFace();

	void setup();
	void update();
	void draw();
	std::vector<ofPoint> getVertices3D(int idx);
	std::vector<ofPoint> getVertices2D(int idx);
	std::vector<ofIndexType> getIndices(int idx);
	ofPoint getHeadPivot3D(int idx);
	ofPoint getHeadPivot2D(int idx);
	float getFaceShapeAnimation(int idx, FaceShapeAnimations unit);

private:
	void processFaces();

	IHighDefinitionFaceFrameSource*	faceFrameSources[BODY_COUNT];
	IHighDefinitionFaceFrameReader*	faceFrameReaders[BODY_COUNT];
	IFaceModelBuilder* faceModelBuilders[BODY_COUNT];
	IFaceModel* faceModel[BODY_COUNT];
	IFaceAlignment* faceAlignment[BODY_COUNT];
	CameraSpacePoint headPivot[BODY_COUNT];
	std::vector<CameraSpacePoint> faceVertices[BODY_COUNT];
	std::vector<ofIndexType> faceIndices[BODY_COUNT];
	float animationUnits[BODY_COUNT][FaceShapeAnimations_Count];

	ofVboMesh vbo[BODY_COUNT];
};