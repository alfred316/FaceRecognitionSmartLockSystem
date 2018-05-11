//Author: Alfred Shaker
//face recognition driven lock controller
//5/3/2018

#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2\face.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <mysql.h>

#include "tserial.h"

using namespace std;
using namespace cv;
using namespace cv::face;

/** Function Headers */
void fisherFaceRecogTrainer();
static void dbread(const string&, vector<Mat>&, vector<int>&, char separator);
void faceRecog();

/** Global variables */
String face_cascade_name = "haarcascade_frontalface_alt.xml";
String eyes_cascade_name = "haarcascade_eye_tree_eyeglasses.xml";
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;
string window_name = "Face Recognition Smart Lock System";

// Serial to Arduino global declarations
int arduino_command;
Tserial *arduino_com;
unsigned char MSB = 0;
// Serial to Arduino global declarations

//mysql things
MYSQL *conn, mysql;
MYSQL_RES *res;
MYSQL_ROW row;

int main(int argc, const char** argv)
{

	//fisherFaceRecogTrainer();

	faceRecog();
	
	return 0;
}



static void dbread(const string& filename, vector<Mat>& images, vector<int>& labels, char separator = ';') {
	std::ifstream file(filename.c_str(), ifstream::in);

	if (!file) {
		string error = "no valid input file";
		//CV_Error(CV_StsBadArg, error);
	}

	string line, path, label;
	while (getline(file, line))
	{
		stringstream liness(line);
		getline(liness, path, separator);
		getline(liness, label);
		if (!path.empty() && !label.empty()) {
			images.push_back(imread(path, 0));
			labels.push_back(atoi(label.c_str()));
		}
	}
}

void faceRecog()
{
	VideoCapture capture;
	Mat frame;

	// serial to Arduino setup 
	arduino_com = new Tserial();
	if (arduino_com != 0) {
		arduino_com->connect("COM3", 57600, spNONE);
	}
	// serial to Arduino setup 

	//load fisher training model
	//Ptr<FisherFaceRecognizer> model = FisherFaceRecognizer::create();

	//model->load<FisherFaceRecognizer>(fileToLoad);

	vector<Mat> images;
	vector<int> labels;

	try {
		string filename = "./at.txt";
		dbread(filename, images, labels);

		cout << "image count is: " << images.size() << endl;
		cout << "label count is: " << labels.size() << endl;
		cout << "The training will now begin!" << endl;

	}
	catch (cv::Exception& e) {
		cerr << "error opening the file" << e.msg << endl;
		exit(1);
	}

	Ptr<FisherFaceRecognizer> model = FisherFaceRecognizer::create();

	try {
		model->train(images, labels);
	}
	catch (cv::Exception& e) {
		cerr << "error training the things: " << e.msg << endl;
		exit(1);
	}


	Mat sampleImg = imread("./at/s41/1.jpg", 0);
	
	int simg_width = sampleImg.cols;
	int simg_height = sampleImg.rows;

	//Load the cascades
	if (!face_cascade.load(face_cascade_name)) { printf("--(!)Error loading\n"); return; };
	if (!eyes_cascade.load(eyes_cascade_name)) { printf("--(!)Error loading\n"); return; };

	//Read the video stream
	capture.open(0);
	if (capture.isOpened())
	{
		namedWindow(window_name, 1);
		long count = 0;

		while (true)
		{

			vector<Rect> faces;
			Mat frame;
			Mat grayScaleFrame;
			Mat original;

			//flag to set to true when unrecognized img saved
			bool checked = false;
			//variable which holds next time to set checked to false if its been set to true
			//current frame + 1000, this is so that it doesnt send a picture and request every frame the unrecognized face
			//is in frame. just once every 1000 or so frames
			int nextCheck = 0;

			capture.read(frame);
			count += 1;
			if (!frame.empty())
			{

				//clone original frame
				original = frame.clone();

				//convert image to gray scale and equalize
				cvtColor(original, grayScaleFrame, COLOR_BGR2GRAY);

				//detect face in gray image
				face_cascade.detectMultiScale(grayScaleFrame, faces, 1.1, 3, 0, cv::Size(90, 90));

				//number of faces detected
				cout << faces.size() << "faces detected: " << endl;
				string frameset = to_string(count);
				string faceset = to_string(faces.size());

				int width = 0, height = 0;

				//recognized person's name
				string rName = "";

				for (int i = 0; i < faces.size(); i++)
				{
					//region of interest
					Rect face_i = faces[i];

					//crop the region of interest from gray image
					Mat face = grayScaleFrame(face_i);

					//resizing the cropped image to suit the database image size
					Mat resizedFace;
					resize(face, resizedFace, Size(simg_width, simg_height), 1.0, 1.0, INTER_CUBIC);

					//recognizing what faces were detected
					int label = -1; double confidence = 0;

					try {
						label = model->predict(resizedFace);
					}
					catch (cv::Exception& e) {
						cerr << "error predicting: " << e.msg << endl;
						exit(1);
					}

					

					//cout << "confidence" << confidence << endl;

					//draw green rect on recognized face
					rectangle(original, face_i, CV_RGB(0, 255, 0), 1);
					string text = "detected and recognized";
					if (label == 40)
					{
						rName = "Alfred";
						//face recognized, send signal to arduino to open lock
						MSB = (1 << 8) & 0xff;
						cout << "CHAR: " << isprint(MSB) << endl;
						arduino_com->sendChar(MSB);
						
						
					}
					else {
						if (checked == false)
						{
							rName = "unknown";
							string newImgName = "newFace" + frameset;
							//save the image locally
							imwrite("./at/new/" + newImgName + ".png", original);
							imwrite("C:/xampp/htdocs/FaceRecLock/images/" + newImgName + ".png", original);

							//set flag "checked" to true for the next 1000 frames, 
							//so set another variable with current frames+1000 so when current frames reach that, 
							//checked is false again
							checked = true;
							nextCheck = count + 1000;
							//save to mysql blob
							//setup mysql connection to localhost phpmyadmin
							const char *server = "localhost";
							const char *user = "root";
							const char *password = "";
							const char *database = "facerec";

							mysql_init(&mysql);
							conn = mysql_real_connect(&mysql, server, user, password, database, 0, 0, 0);
							if (conn == NULL)
							{
								cout << mysql_error(&mysql) << endl << endl;
								return;
							}
							string query = "INSERT INTO newface (img) VALUES ('C:/xampp/htdocs/FaceRecLock/images/" + newImgName + ".png')";
							//query
							mysql_query(conn, query.c_str());

							//
						}
						else
						{
							//if checked is true, meaning an unrecognized face has been registered
							//make sure it's been 1000 frames since it's been set to true
							if (count >= nextCheck)
							{
								//if it is, set it back to false so that the next frame can capture the face and send it
								checked = false;
							}
						}
						

					}

					int pos_x = max(face_i.tl().x - 10, 0);
					int pos_y = max(face_i.tl().y - 10, 0);

					//name the person in the image
					putText(original, text, Point(pos_x, pos_y), FONT_HERSHEY_COMPLEX, 1.0, CV_RGB(0, 255, 0), 1.0);

				}

				putText(original, "Frames: " + frameset, Point(30, 60), FONT_HERSHEY_COMPLEX, 1.0, CV_RGB(0, 255, 0), 1.0);
				putText(original, "Person: " + rName, Point(30, 90), FONT_HERSHEY_COMPLEX, 1.0, CV_RGB(0, 255, 0), 1.0);

				imshow(window_name, original);

			}
			else
			{
				printf(" --(!) No captured frame -- Break!"); break;
			}

			int c = waitKey(10);
			if ((char)c == 'c') { break; }
		}
	}
	// Serial to Arduino - shutdown
	arduino_com->disconnect();
	delete arduino_com;
	arduino_com = 0;
	// Serial to Arduino - shutdown
}

void fisherFaceRecogTrainer()
{
	vector<Mat> images;
	vector<int> labels;

	try {
		string filename = "./at.txt";
		dbread(filename, images, labels);

		cout << "image count is: " << images.size() << endl;
		cout << "label count is: " << labels.size() << endl;
		cout << "The training will now begin!" << endl;

	}
	catch (cv::Exception& e) {
		cerr << "error opening the file" << e.msg << endl;
		exit(1);
	}
	
	Ptr<FisherFaceRecognizer> model = FisherFaceRecognizer::create();

	try {
		model->train(images, labels);
	}
	catch (cv::Exception& e) {
		cerr << "error training the things: " << e.msg << endl;
		exit(1);
	}

	

	int height = images[0].rows;

	try {
		model->save("fisherface.yml");
	}
	catch(cv::Exception& e){
		cerr << "error saving to the file: " << e.msg << endl;
		exit(1);
	}

	

	cout << "training finished..." << endl;
}