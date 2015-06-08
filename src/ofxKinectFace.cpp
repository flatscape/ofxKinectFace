//
//  ofxKinectFace
//
//  Created by flatscape
//
//  Released under the MIT license
//  http://opensource.org/licenses/mit-license.php
//
#include "ofxKinectFace.h"

#pragma mark - KinectBase

static const int COLOR_WIDTH = 1920;
static const int COLOR_HEIGHT = 1080;
static const DWORD FACE_FRAME_FEATURES = 
	FaceFrameFeatures::FaceFrameFeatures_BoundingBoxInColorSpace
	| FaceFrameFeatures::FaceFrameFeatures_PointsInColorSpace
	| FaceFrameFeatures::FaceFrameFeatures_RotationOrientation
	| FaceFrameFeatures::FaceFrameFeatures_Happy
	| FaceFrameFeatures::FaceFrameFeatures_RightEyeClosed
	| FaceFrameFeatures::FaceFrameFeatures_LeftEyeClosed
	| FaceFrameFeatures::FaceFrameFeatures_MouthOpen
	| FaceFrameFeatures::FaceFrameFeatures_MouthMoved
	| FaceFrameFeatures::FaceFrameFeatures_LookingAway
	| FaceFrameFeatures::FaceFrameFeatures_Glasses
	| FaceFrameFeatures::FaceFrameFeatures_FaceEngagement;
static const ofColor* FACE_COLOR[] =
{
	&ofColor::red,
	&ofColor::green,
	&ofColor::white,
	&ofColor::purple,
	&ofColor::orange,
	&ofColor::pink,
};

KinectBase::KinectBase()
{
	color = new ofImage();
	color->allocate(COLOR_WIDTH, COLOR_HEIGHT, OF_IMAGE_COLOR_ALPHA);
	for (int i = 0; i < BODY_COUNT; i++)
	{
		isFaceValid[i] = false;
	}
}

KinectBase::~KinectBase()
{
	close();
}

bool KinectBase::setup()
{
	HRESULT hr = GetDefaultKinectSensor(&sensor);

	if(sensor)
	{
		IColorFrameSource*	colorFrameSource = nullptr;
		IBodyFrameSource*	bodyFrameSource = nullptr;

		hr = sensor->Open();

		if (SUCCEEDED(hr))
		{
			hr = sensor->get_ColorFrameSource(&colorFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = colorFrameSource->OpenReader(&colorFrameReader);
		}

		if (SUCCEEDED(hr))
		{
			hr = sensor->get_BodyFrameSource(&bodyFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = bodyFrameSource->OpenReader(&bodyFrameReader);
		}

		if (SUCCEEDED(hr)){
			hr = sensor->get_CoordinateMapper( &coordinateMapper );
		}

		SafeRelease(colorFrameSource);
		SafeRelease(bodyFrameSource);
	}

	return SUCCEEDED(hr);
}

void KinectBase::update()
{
	if(!colorFrameReader || !bodyFrameReader)
	{
		return;
	}

	IColorFrame* colorFrame = NULL;
	HRESULT hr = colorFrameReader->AcquireLatestFrame(&colorFrame);

	if(SUCCEEDED(hr))
	{
		INT64 nTime = 0;
		hr = colorFrame->get_RelativeTime(&nTime);

		IFrameDescription* description = NULL;
		int width = 0;
		int height = 0;
		ColorImageFormat imageFormat = ColorImageFormat_None;
		BYTE *pixels;
		UINT bufferSize = 0;

		if (SUCCEEDED(hr))
		{
			hr = colorFrame->get_FrameDescription(&description);
		}

		if (SUCCEEDED(hr))
		{
			hr = description->get_Width(&width);
		}

		if (SUCCEEDED(hr))
		{
			hr = description->get_Height(&height);
		}

		if (SUCCEEDED(hr))
		{
			hr = colorFrame->get_RawColorImageFormat(&imageFormat);
		}

		if (SUCCEEDED(hr))
		{
			if (width == COLOR_WIDTH && height == COLOR_HEIGHT)
			{
				if (imageFormat == ColorImageFormat_Rgba)
				{
					pixels = color->getPixels();
					hr = colorFrame->AccessRawUnderlyingBuffer(&bufferSize, &pixels);
				}
				else
				{
					pixels = color->getPixels();
					bufferSize = COLOR_WIDTH * COLOR_HEIGHT * 4;
					hr = colorFrame->CopyConvertedFrameDataToArray(bufferSize, pixels, ColorImageFormat_Rgba);
				}
				if (SUCCEEDED(hr))
				{
					color->update();
					processFaces();
				}
			}
		}
		SafeRelease(description);
	}

	SafeRelease(colorFrame);
}

void KinectBase::drawColor(int x, int y)
{
	color->draw(x, y);
}

void KinectBase::drawColor(int x, int y, int w, int h)
{
	color->draw(x, y, w, h);
}

void KinectBase::close()
{
	if (!sensor) {
		return;
	}

	sensor->Close();
	sensor = nullptr;
}

ofQuaternion KinectBase::getRotation(int idx)
{
	return (idx>=0 && idx<BODY_COUNT) ? ofQuaternion(faceRotation[idx].x, faceRotation[idx].y, faceRotation[idx].z, faceRotation[idx].w) : ofQuaternion();
}

bool KinectBase::getIsFaceValid(int idx)
{
	return (idx>=0 && idx<BODY_COUNT) ? isFaceValid[idx] : false;
}

int KinectBase::getBodyCount()
{
	return BODY_COUNT;
}

int KinectBase::getWidth()
{
	return color->getWidth();
}

int KinectBase::getHeight()
{
	return color->getHeight();
}

bool KinectBase::updateBodyData(IBody** bodies)
{
	HRESULT hr = E_FAIL;

	if (bodyFrameReader != nullptr)
	{
		IBodyFrame* pBodyFrame = nullptr;
		hr = bodyFrameReader->AcquireLatestFrame(&pBodyFrame);
		if (SUCCEEDED(hr))
		{
			hr = pBodyFrame->GetAndRefreshBodyData(BODY_COUNT, bodies);
		}
		SafeRelease(pBodyFrame);
	}

	return SUCCEEDED(hr);
}

ColorSpacePoint KinectBase::cameraToScreen(CameraSpacePoint pp)
{
	ColorSpacePoint np;
	HRESULT hr = coordinateMapper->MapCameraPointToColorSpace( pp, &np );
	return np;
}

#pragma mark - ofxKinectFace

ofxKinectFace::ofxKinectFace()
{
}

void ofxKinectFace::setup(){
	HRESULT hr = E_FAIL;

	if (KinectBase::setup())
	{
		hr = NOERROR;
		for (int i = 0; i < BODY_COUNT; i++)
		{
			if (SUCCEEDED(hr))
			{
				hr = CreateFaceFrameSource(sensor, 0, FACE_FRAME_FEATURES, &faceFrameSources[i]);
			}
			if (SUCCEEDED(hr))
			{
				hr = faceFrameSources[i]->OpenReader(&faceFrameReaders[i]);
			}				
		}
	}

	if (!sensor || FAILED(hr))
	{
		ofLogError("No ready Kinect found!");
	}
}

void ofxKinectFace::update(){
	KinectBase::update();
}

void ofxKinectFace::draw(){
	for (int i = 0; i < BODY_COUNT; ++i)
	{
		if(!isFaceValid[i])continue;

		ofSetColor(*FACE_COLOR[i]);
		ofNoFill();
		ofSetLineWidth(3);
		ofRect(faceRect[i]);
		ofFill();
		for (int j = 0; j < FacePointType::FacePointType_Count; j++)
		{
			ofCircle(facePoints[i][j].X, facePoints[i][j].Y, 5);
		}

		std::string text;

		for (int j = 0; j < FaceProperty::FaceProperty_Count; j++)
		{
			switch (j)
			{
			case FaceProperty::FaceProperty_Happy:
				text += "Happy :";
				break;
			case FaceProperty::FaceProperty_Engaged:
				text += "Engaged :";
				break;
			case FaceProperty::FaceProperty_LeftEyeClosed:
				text += "LeftEyeClosed :";
				break;
			case FaceProperty::FaceProperty_RightEyeClosed:
				text += "RightEyeClosed :";
				break;
			case FaceProperty::FaceProperty_LookingAway:
				text += "LookingAway :";
				break;
			case FaceProperty::FaceProperty_MouthMoved:
				text += "MouthMoved :";
				break;
			case FaceProperty::FaceProperty_MouthOpen: 
				text += "MouthOpen :";
				break;
			case FaceProperty::FaceProperty_WearingGlasses:
				text += "WearingGlasses :";
				break;
			default:
				break;
			}

			switch (faceProperties[i][j]) 
			{
			case DetectionResult::DetectionResult_Unknown:
				text += " UnKnown";
				break;
			case DetectionResult::DetectionResult_Yes:
				text += " Yes";
				break;
			case DetectionResult::DetectionResult_No:
			case DetectionResult::DetectionResult_Maybe:
				text += " No";
				break;
			default:
				break;
			}

			text += "\n";
		}

		float x = faceRotation[i].x;
		float y = faceRotation[i].y;
		float z = faceRotation[i].z;
		float w = faceRotation[i].w;
		float pitch = atan2(2 * (y * z + w * x), w * w - x * x - y * y + z * z) / PI * 180.f;
		float yaw = asin(2 * (w * y - x * z)) / PI * 180.f;
		float roll = atan2(2 * (x * y + w * z), w * w + x * x - y * y - z * z) / PI * 180.f;

		text += "FaceYaw : " + ofToString((int)yaw) + "\n";
		text += "FacePitch : " + ofToString((int)pitch) + "\n";
		text += "FaceRoll : " + ofToString((int)roll) + "\n";

		ofDrawBitmapString(text, faceRect[i].x, faceRect[i].y+faceRect[i].height+20);
	}
};

ofRectangle ofxKinectFace::getFaceRect(int idx)
{
	return (idx>=0 && idx<BODY_COUNT) ? faceRect[idx] : ofRectangle();
}

ofPoint ofxKinectFace::getFacePoint(int idx, FacePointType type)
{
	return (idx>=0 && idx<BODY_COUNT) ? ofPoint(facePoints[idx][type].X, facePoints[idx][type].Y) : ofPoint();
}

DetectionResult ofxKinectFace::getFaceProperty(int idx, FaceProperty type)
{
	return (idx>=0 && idx<BODY_COUNT) ? faceProperties[idx][type] : DetectionResult_Unknown;
}

void ofxKinectFace::processFaces()
{
	HRESULT hr;
	IBody* bodies[BODY_COUNT] = {0};
	bool haveBodyData = updateBodyData(bodies);

	for (int i = 0; i < BODY_COUNT; ++i)
	{
		isFaceValid[i] = false;

		IFaceFrame* faceFrame = nullptr;
		hr = faceFrameReaders[i]->AcquireLatestFrame(&faceFrame);

		BOOLEAN trackingIdValid = FALSE;
		if (SUCCEEDED(hr) && nullptr != faceFrame)
		{
			hr = faceFrame->get_IsTrackingIdValid(&trackingIdValid);
		}

		if (SUCCEEDED(hr))
		{
			if (trackingIdValid)
			{
				IFaceFrameResult* faceFrameResult = nullptr;
				RectI faceBox = {0};

				hr = faceFrame->get_FaceFrameResult(&faceFrameResult);

				if (SUCCEEDED(hr) && faceFrameResult != nullptr)
				{
					hr = faceFrameResult->get_FaceBoundingBoxInColorSpace(&faceBox);

					if (SUCCEEDED(hr))
					{			
						hr = faceFrameResult->GetFacePointsInColorSpace(FacePointType::FacePointType_Count, facePoints[i]);
					}

					if (SUCCEEDED(hr))
					{
						hr = faceFrameResult->get_FaceRotationQuaternion(&faceRotation[i]);
					}

					if (SUCCEEDED(hr))
					{			
						faceRect[i].set(faceBox.Left, faceBox.Top, faceBox.Right-faceBox.Left, faceBox.Bottom-faceBox.Top);

						bool isValid = true;
						if(faceRect[i].x <= 0 || faceRect[i].x > ofGetWidth())isValid = false;
						if(faceRect[i].y <= 0 || faceRect[i].y > ofGetHeight())isValid = false;
						for (int j = 0; j < FacePointType::FacePointType_Count; j++)
						{
							if(facePoints[i][j].X <= 0 || facePoints[i][j].X > ofGetWidth())isValid = false;
							if(facePoints[i][j].Y <= 0 || facePoints[i][j].Y > ofGetHeight())isValid = false;
						}
						if(faceRotation[i].x == 0.f && faceRotation[i].y == 0.f && faceRotation[i].z == 0.f && faceRotation[i].w == 0.f)isValid = false;
						if(isValid)isFaceValid[i] = true;
					}

					if (SUCCEEDED(hr))
					{
						hr = faceFrameResult->GetFaceProperties(FaceProperty::FaceProperty_Count, faceProperties[i]);
					}
				}

				SafeRelease(faceFrameResult);	
			}
			else
			{
				if (haveBodyData)
				{
					IBody* pBody = bodies[i];
					if (pBody != nullptr)
					{
						BOOLEAN tracked = false;
						hr = pBody->get_IsTracked(&tracked);

						UINT64 trackID;
						if (SUCCEEDED(hr) && tracked)
						{
							hr = pBody->get_TrackingId(&trackID);
							if (SUCCEEDED(hr))
							{
								faceFrameSources[i]->put_TrackingId(trackID);
							}
						}
					}
				}
			}
		}
	}

	if (haveBodyData)
	{
		for (int i = 0; i < _countof(bodies); ++i)
		{
			SafeRelease(bodies[i]);
		}
	}
}

#pragma mark - ofxKinectHDFace

ofxKinectHDFace::ofxKinectHDFace()
{
	for (int i = 0; i < BODY_COUNT; i++)
	{
		for (int j = 0; j < FaceShapeAnimations_Count; j++)
		{
			animationUnits[i][j] = 0.f;
		}
	}
}

void ofxKinectHDFace::setup(){
	HRESULT hr = E_FAIL;

	if (KinectBase::setup())
	{
		hr = NOERROR;
		for (int i = 0; i < BODY_COUNT; i++)
		{
			if (SUCCEEDED(hr))
			{
				hr = CreateHighDefinitionFaceFrameSource(sensor, &faceFrameSources[i]);
			}
			if (SUCCEEDED(hr))
			{
				hr = faceFrameSources[i]->OpenReader(&faceFrameReaders[i]);
			}
			if (SUCCEEDED(hr))
			{
				hr = faceFrameSources[i]->OpenModelBuilder( FaceModelBuilderAttributes::FaceModelBuilderAttributes_None, &faceModelBuilders[i] );
			}
			if (SUCCEEDED(hr))
			{
				hr = faceModelBuilders[i]->BeginFaceDataCollection();
			}
			if (SUCCEEDED(hr))
			{
				hr = CreateFaceAlignment( &faceAlignment[i] );
			}
			if (SUCCEEDED(hr))
			{
				std::vector<float> deformations( FaceShapeDeformations::FaceShapeDeformations_Count, 0.f );
				hr = CreateFaceModel( 1.0f, FaceShapeDeformations::FaceShapeDeformations_Count, &deformations[0], &faceModel[i] );
			}
		}
	}

	if (!sensor || FAILED(hr))
	{
		ofLogError("No ready Kinect found!");
	}

	UINT32 vertexCount = 0;
	hr = GetFaceModelVertexCount( &vertexCount );
	if (SUCCEEDED(hr))
	{
		for (int i = 0; i < BODY_COUNT; i++)
		{
			faceVertices[i].assign(vertexCount, CameraSpacePoint());
		}

		UINT32 triangleCount = 0;
		hr = GetFaceModelTriangleCount(&triangleCount);
		if (SUCCEEDED(hr))
		{
			vector<UINT32> triangles;
			triangles.assign(triangleCount*3, 0);
			hr = GetFaceModelTriangles(triangles.size(), &triangles[0]);
			if (SUCCEEDED(hr))
			{
				for (int i = 0; i < BODY_COUNT; i++)
				{
					for (int j = 0; j < triangles.size(); j++)
					{
						faceIndices[i].push_back(triangles[j]);
					}
					vbo[i].addVertices(getVertices3D(i));
					vbo[i].addIndices(faceIndices[i]);
				}
			}
		}

	}
}

void ofxKinectHDFace::update(){
	KinectBase::update();
}

void ofxKinectHDFace::draw(){
	for (int i = 0; i < BODY_COUNT; i++)
	{
		//if(!isFaceValid[i])continue;

		for (int j = 0; j < faceVertices[i].size(); j++)
		{
			ColorSpacePoint p = cameraToScreen(faceVertices[i][j]);
			vbo[i].setVertex(j, ofPoint(p.X, p.Y));
		}
		vbo[i].drawWireframe();
	}
}

std::vector<ofPoint> ofxKinectHDFace::getVertices3D(int idx)
{
	if(idx<0 || idx>=BODY_COUNT)return std::vector<ofPoint>();

	std::vector<ofPoint> vertices3D(faceVertices[idx].size());
	for (int i = 0; i < faceVertices[idx].size(); i++)
	{
		vertices3D[i].x = faceVertices[idx][i].X;
		vertices3D[i].y = faceVertices[idx][i].Y;
		vertices3D[i].z = faceVertices[idx][i].Z;
	}
	return vertices3D;
}

std::vector<ofPoint> ofxKinectHDFace::getVertices2D(int idx)
{
	if(idx<0 || idx>=BODY_COUNT)return std::vector<ofPoint>();

	std::vector<ofPoint> vertices2D(faceVertices[idx].size());
	for (int i = 0; i < faceVertices[idx].size(); i++)
	{
		ColorSpacePoint p = cameraToScreen(faceVertices[idx][i]);
		vertices2D[i].x = p.X;
		vertices2D[i].y = p.Y;
	}
	return vertices2D;
}

std::vector<ofIndexType> ofxKinectHDFace::getIndices(int idx)
{
	return (idx>=0 && idx<BODY_COUNT) ? faceIndices[idx] : std::vector<ofIndexType>();
}

ofPoint ofxKinectHDFace::getHeadPivot3D(int idx)
{
	return (idx>=0 && idx<BODY_COUNT) ? ofPoint(headPivot[idx].X, headPivot[idx].Y, headPivot[idx].Z) : ofPoint();
}

ofPoint ofxKinectHDFace::getHeadPivot2D(int idx)
{
	if(idx<0 || idx>=BODY_COUNT)return ofPoint();

	ColorSpacePoint p = cameraToScreen(headPivot[idx]);
	return ofPoint(p.X, p.Y);
}

float ofxKinectHDFace::getFaceShapeAnimation(int idx, FaceShapeAnimations unit)
{
	if(idx<0 || idx>=BODY_COUNT)return 0;

	return animationUnits[idx][unit];
}

void ofxKinectHDFace::processFaces()
{
	HRESULT hr;
	IBody* bodies[BODY_COUNT] = { 0 };
	bool haveBodyData = SUCCEEDED(updateBodyData(bodies));

	for (int i = 0; i < BODY_COUNT; i++)
	{
		isFaceValid[i] = false;

		IHighDefinitionFaceFrame * faceFrame = nullptr;
		hr = faceFrameReaders[i]->AcquireLatestFrame(&faceFrame);

		BOOLEAN trackingIdValid = FALSE;
		if (SUCCEEDED(hr) && nullptr != faceFrame)
		{
			hr = faceFrame->get_IsTrackingIdValid(&trackingIdValid);
		}

		if (SUCCEEDED(hr) && trackingIdValid)
		{
			hr = faceFrame->GetAndRefreshFaceAlignmentResult(faceAlignment[i]);

			if(SUCCEEDED(hr) && faceAlignment[i] != nullptr)
			{
				hr = faceAlignment[i]->GetAnimationUnits(FaceShapeAnimations_Count, animationUnits[i]);

				if (SUCCEEDED(hr))
				{
					hr = faceModel[i]->CalculateVerticesForAlignment(faceAlignment[i], faceVertices[i].size(), &faceVertices[i][0]);
				}

				if (SUCCEEDED(hr))
				{
					hr = faceAlignment[i]->get_HeadPivotPoint(&headPivot[i]);
				}

				if (SUCCEEDED(hr))
				{
					hr = faceAlignment[i]->get_FaceOrientation(&faceRotation[i]);
				}

				if (SUCCEEDED(hr))
				{
					bool isValid = true;
					ofPoint headPivot = getHeadPivot2D(i);
					if(headPivot.x <= 0 || headPivot.x > ofGetWidth())isValid = false;
					if(headPivot.y <= 0 || headPivot.y > ofGetHeight())isValid = false;
					if(faceRotation[i].x == 0.f && faceRotation[i].y == 0.f && faceRotation[i].z == 0.f && faceRotation[i].w == 0.f)isValid = false;
					if(isValid)isFaceValid[i] = true;
				}

			}
		}
		else
		{
			if (haveBodyData)
			{
				IBody* pBody = bodies[i];
				if (pBody != nullptr)
				{
					BOOLEAN tracked = false;
					hr = pBody->get_IsTracked(&tracked);

					UINT64 trackID;
					if (SUCCEEDED(hr) && tracked)
					{
						hr = pBody->get_TrackingId(&trackID);
						if (SUCCEEDED(hr))
						{
							faceFrameSources[i]->put_TrackingId(trackID);
						}
					}
				}
			}
		}

		SafeRelease(faceFrame);
	}

	if (haveBodyData)
	{
		for (int i = 0; i < _countof(bodies); ++i)
		{
			SafeRelease(bodies[i]);
		}
	}
}