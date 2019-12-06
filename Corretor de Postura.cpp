#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <exception>
#include <Windows.h>
#include <fstream>
#include <conio.h>
#include <MMSystem.h>

#define ESC 80


using namespace std;
using namespace cv;

string cascadeName;
const char* title = "Corretor de postura";
Mat fruta;
int flag = 0, cont = 0, posIrregulares = 0, posMax = 0, posMin = 190, posMed = 0;
long long int somaPos = 0, qtdPos = 0;
int tecla;

void salvaMedidas(int posMax, int posMin, int total)
{
	ofstream salvaArquivo;
	salvaArquivo.open("medias.txt", ios::out);

	// Verifica se o arquivo foi aberto corretamente
	if (!salvaArquivo.is_open()) {
		cout << "Nao foi possivel abrir arquivo!!\n";
		salvaArquivo.close();
		return;
	}

	salvaArquivo << "Posicao Maximo: " << posMax << endl;
	salvaArquivo << "Posicao Minima: " << posMin << endl;
	salvaArquivo << "Total de posicao irregulares: " << total << endl;

	salvaArquivo.close();
}

/**
 * @brief Draws a transparent image over a frame Mat.
 *
 * @param frame the frame where the transparent image will be drawn
 * @param transp the Mat image with transparency, read from a PNG image, with the IMREAD_UNCHANGED flag
 * @param xPos x position of the frame image where the image will start.
 * @param yPos y position of the frame image where the image will start.
 */
void drawTransparency(Mat frame, Mat transp, int xPos, int yPos) {
	Mat mask;
	vector<Mat> layers;

	split(transp, layers); // seperate channels
	Mat rgb[3] = { layers[0],layers[1],layers[2] };
	mask = layers[3]; // png's alpha channel used as mask
	merge(rgb, 3, transp);  // put together the RGB channels, now transp insn't transparent 
	transp.copyTo(frame.rowRange(yPos, yPos + transp.rows).colRange(xPos, xPos + transp.cols), mask);
}

void drawTransparency2(Mat frame, Mat transp, int xPos, int yPos) {
	Mat mask;
	vector<Mat> layers;

	split(transp, layers); // seperate channels
	Mat rgb[3] = { layers[0],layers[1],layers[2] };
	mask = layers[3]; // png's alpha channel used as mask
	merge(rgb, 3, transp);  // put together the RGB channels, now transp insn't transparent 
	Mat roi1 = frame(Rect(xPos, yPos, transp.cols, transp.rows));
	Mat roi2 = roi1.clone();
	transp.copyTo(roi2.rowRange(0, transp.rows).colRange(0, transp.cols), mask);
	//printf("%p, %p\n", roi1.data, roi2.data);
	double alpha = 0.2;
	addWeighted(roi2, alpha, roi1, 1.0 - alpha, 0.0, roi1);
}

void detectAndDraw(Mat & img, CascadeClassifier & face_cascade, double scale)
{
	std::vector<Rect> faces;
	Scalar color = Scalar(255, 0, 0);
	Mat gray, smallImg;

	cvtColor(img, gray, COLOR_BGR2GRAY);
	double fx = 1 / scale;
	resize(gray, smallImg, Size(), fx, fx, INTER_LINEAR);
	equalizeHist(smallImg, smallImg);

	face_cascade.detectMultiScale(smallImg, faces,
		1.3, 2, 0
		//|CASCADE_FIND_BIGGEST_OBJECT
		//|CASCADE_DO_ROUGH_SEARCH
		| CASCADE_SCALE_IMAGE,
		Size(40, 40));

	for (size_t i = 0; i < faces.size(); i++)
	{
		Rect r = faces[i];
		Point center;

		if (r.y >= posMax)
		{
			posMax = r.y;
		}
		if (r.y <= posMin)
		{
			posMin = r.y;
		}

		if (r.y <= 90 || r.y >= 190) {
			puts("Posicao irregular");
			if (!flag) {
				PlaySound(TEXT("ntfpluck.wav"), NULL, SND_ASYNC);
				flag = 1;
				posIrregulares++;
				cout<<"total= "<<posIrregulares<<endl;

			}else {
				cont += 1;
				if (cont == 10) {
					flag = 0;
					cont = 0;
				}
			}
			somaPos += r.y;
			qtdPos++;

		}

		salvaMedidas(posMax, posMin, posIrregulares);

		//printf("xy face = %d x %d\n", r.x, r.y);


		rectangle(img, Point(cvRound(r.x * scale), cvRound(r.y * scale)),
			Point(cvRound((r.x + r.width - 1) * scale), cvRound((r.y + r.height - 1) * scale)),
			color, 3, 8, 0);


	}
	imshow(title, img);

}

int main(int argc, const char** argv)
{
	VideoCapture capture;
	Mat frame;
	CascadeClassifier face_cascade;
	double scale;

	CommandLineParser parser(argc, argv,
		"{help h||}"
		"{face_cascade|C:/Users/pablo/Source/Repos/vcpkg/downloads/opencv-4.1.1/data/haarcascades/haarcascade_frontalface_alt.xml|Path to face cascade.}"
		"{camera|0|Camera device number.}");

	parser.about("\nThis program demonstrates using the cv::CascadeClassifier class to detect objects (Face + eyes) in a video stream.\n"
		"You can use Haar or LBP features.\n\n");
	parser.printMessage();

	String face_cascade_name = parser.get<String>("face_cascade");

	//-- 1. Load the cascades
	if (!face_cascade.load(face_cascade_name))
	{
		cout << "--(!)Error loading face cascade\n";
		return -1;
	};

	scale = 1;

	try {
		//const char * device = "/dev/video0";
		//const char * device = "rtsp://192.168.43.1:8080/h264_ulaw.sdp";
		int camera_device = parser.get<int>("camera");
		if (!capture.open(camera_device))
			cout << "Capture from camera #0 didn't work" << endl;
	}
	catch (std::exception & e)
	{
		std::cout << " Excecao capturada open: " << e.what() << std::endl;
	}

	if (capture.isOpened())
	{
		cout << "Video capturing has been started ..." << endl;
		namedWindow(title, cv::WINDOW_NORMAL);
		for (;;)
		{
			try {
				capture >> frame;
			}
			catch (cv::Exception & e)
			{
				std::cout << " Excecao2 capturada frame: " << e.what() << std::endl;
				continue;
			}
			catch (std::exception & e)
			{
				std::cout << " Excecao3 capturada frame: " << e.what() << std::endl;
				continue;
			}

			if (frame.empty())
				break;


			/*for (int m=0 ; m<400 ; m++) {
				//unsigned char * p = frame.ptr(m, m);
				p[0] = 255;
				p[1] = 0;
				p[2] = 0;
			}*/

			detectAndDraw(frame, face_cascade, scale);

			char c = (char)waitKey(10);
			if (c == 27 || c == 'q' || c == 'Q')
				break;
		}
	}

	return 0;
}




