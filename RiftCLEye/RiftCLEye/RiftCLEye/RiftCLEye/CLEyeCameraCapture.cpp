#include "CLEyeCameraCapture.h"


CLEyeCameraCapture::CLEyeCameraCapture()
{
}

CLEyeCameraCapture::CLEyeCameraCapture(LPSTR windowName, GUID cameraGUID, CLEyeCameraColorMode mode, CLEyeCameraResolution resolution, float fps)
	: m_cameraGUID(cameraGUID), m_cam(NULL), m_mode(mode), m_resolution(resolution), m_fps(fps), m_running(false)
{
	strcpy_s(_windowName, windowName);
}

CLEyeCameraCapture::~CLEyeCameraCapture()
{
}

bool CLEyeCameraCapture::StartCapture()
{
	m_running = true;
	cvNamedWindow(_windowName, CV_WINDOW_AUTOSIZE);

	m_hThread = CreateThread(NULL, 0, &CLEyeCameraCapture::CaptureThread, this, 0, 0);
	if (m_hThread == NULL)
	{
		MessageBoxW(NULL, (LPCWSTR)"Could not create capture thread", (LPCWSTR)"RiftCLEye", MB_ICONEXCLAMATION);
		return false;
	}
	return true;
}

void CLEyeCameraCapture::StopCapture()
{
	if (!m_running) return;
	m_running = false;
	WaitForSingleObject(m_hThread, 1000);
	cvDestroyWindow(_windowName);
}

void CLEyeCameraCapture::Run()
{
	int w, h;
	IplImage *pCapImage;
	PBYTE pCapBuffer = NULL;

	// Ceate camera instance
	m_cam = CLEyeCreateCamera(m_cameraGUID, m_mode, m_resolution, m_fps);
	if (m_cam == NULL)	return;
	// Get camera frame dimensions
	CLEyeCameraGetFrameDimensions(m_cam, w, h);
	// Depending on color mode chose, create the appropriate OpenCV image
	if (m_mode == CLEYE_COLOR_PROCESSED || m_mode == CLEYE_COLOR_RAW)
		pCapImage = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 4);
	else
		pCapImage = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 1);

	// Set some camera parameters
	CLEyeSetCameraParameter(m_cam, CLEYE_GAIN, 0);
	CLEyeSetCameraParameter(m_cam, CLEYE_EXPOSURE, 511);
	//CLEyeSetCameraParameter(m_cam, CLEYE_ZOOM, 0);
	//CLEyeSetCameraParameter(m_cam, CLEYE_ROTATION, 0);

	// Start capture
	CLEyeCameraStart(m_cam);
	cvGetImageRawData(pCapImage, &pCapBuffer);
	// image capturing loop
	while (m_running)
	{
		CLEyeCameraGetFrame(m_cam, pCapBuffer);
		cvShowImage(_windowName, pCapImage);
	}
	// Stop camera capture
	CLEyeCameraStop(m_cam);
	CLEyeDestroyCamera(m_cam);
	cvReleaseImage(&pCapImage);
	m_cam = NULL;
}

DWORD WINAPI CLEyeCameraCapture::CaptureThread(LPVOID instance)
{
	srand(GetTickCount() + GetCurrentThreadId());
	CLEyeCameraCapture *pThis = (CLEyeCameraCapture *)instance;
	pThis->Run();
	return 0;
}
