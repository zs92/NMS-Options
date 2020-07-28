#include <iostream>

#include <QtCore>
#include <QString>

#include "SHRectangle.h"
#include "SHColor.h"

SH::DetectionRectangle readZBSAnnotationLine(const QString& annotationLine) {
	SH::DetectionRectangle result;

	QRegExp rx("\\<(.*)\\>");
	int pos = rx.indexIn(annotationLine);
	QString className = rx.capturedTexts().back();
	QString values = annotationLine.split(QRegExp("\\<.*\\>"), QString::SkipEmptyParts).back();
	QStringList valuesSplit = values.split(QRegExp("\\s+"), QString::SkipEmptyParts);

	if (rx.capturedTexts().size() != 2 ||
			annotationLine.split(QRegExp("\\<.*\\>"), QString::SkipEmptyParts).size() != 1 ||
			valuesSplit.size() < 4)
	{
		std::cout << __func__ << ": Error parsing annotationLine" << std::endl;
		return result;
	}

	result.className = className;
	result.x_c = valuesSplit.at(0).toFloat();
	result.y_c = valuesSplit.at(1).toFloat();
	result.width = valuesSplit.at(2).toFloat();
	result.height = valuesSplit.at(3).toFloat();
	if (valuesSplit.size() > 4) {
		result.prob = valuesSplit.at(4).toFloat();
	}

	return result;
}





std::vector<SH::DetectionRectangle> readZBSAnnotationsFile(QString ZBSAnnotationsFilePath) {
	std::vector<SH::DetectionRectangle> result;

	QFile ZBSAnnotationsFile(ZBSAnnotationsFilePath);
	if (!ZBSAnnotationsFile.open(QIODevice::ReadOnly)) {
		return result;
	}

	QTextStream inStream(&ZBSAnnotationsFile);
	while (!inStream.atEnd()) {
		QString annotationLine = inStream.readLine();
		SH::DetectionRectangle detection = readZBSAnnotationLine(annotationLine);
		if (!detection.className.isEmpty()) {
			result.push_back(detection);
		}
	}
	return result;
}


void writeZBSAnnotationsFile(std::vector<SH::DetectionRectangle> detections, QString ZBSAnnotationsFilePath) {
	if (ZBSAnnotationsFilePath.isEmpty()) {
		return;
	}

	QFile annotationFile(ZBSAnnotationsFilePath);
	if (!annotationFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return;
	}

	QTextStream zbsAnnotationsOutStream(&annotationFile);
	for (auto detection : detections) {
		zbsAnnotationsOutStream << "<" << detection.className << "> "
			<< detection.x_c << " "
			<< detection.y_c << " "
			<< detection.width << " "
			<< detection.height << " "
			<< detection.prob << "\n";
	}
}


void cvDrawGradientLine(cv::Mat& img, const cv::Point& start, const cv::Point& end,
												const cv::Scalar& c1, const cv::Scalar& c2, int thickness = 1)
{
	cv::LineIterator iter(img, start, end, cv::LINE_8);
	for (int i = 0; i < iter.count; i++, iter++) {
		if (iter.pos().x - thickness >= 0 &&
				iter.pos().y - thickness >= 0 &&
				iter.pos().x + thickness < img.cols &&
				iter.pos().y + thickness < img.rows)
		{
			double alpha = double(i) / iter.count;
			img(cv::Rect(iter.pos(), cv::Size(thickness, thickness))) = c1 * (1.0 - alpha) + c2 * alpha;
		}
	}
}


void drawKeypoints(cv::Mat& img_draw, std::vector<SH::DetectionRectangle> detections) {
	{ // Draw Rectangles
		cv::Scalar color;
		color = cv::Scalar(195, 195, 195, 255);

		// Draw all rectangles in vector
		for (auto rect : detections) {
			QString firstClassName = rect.className;
			if (firstClassName.contains("/")) { // Keypoint detection -- Draw "/" limb
				QStringList partsList = firstClassName.split('/');
				if (partsList.size() == 2) {
					cvDrawGradientLine(img_draw,
														 cv::Point(rect.getLeftBorder() * img_draw.cols, rect.getBottomBorder() * img_draw.rows),
														 cv::Point(rect.getRightBorder() * img_draw.cols, rect.getTopBorder() * img_draw.rows),
														 SH::makeColor(partsList[0].toInt()), SH::makeColor(partsList[1].toInt()), 2);
				}
			} else if (rect.className.contains("\\")) { // Keypoint detection -- Draw "\" limb
				QStringList partsList = firstClassName.split('\\');
				if (partsList.size() == 2) {
					cvDrawGradientLine(img_draw,
														 cv::Point(rect.getLeftBorder() * img_draw.cols, rect.getTopBorder() * img_draw.rows),
														 cv::Point(rect.getRightBorder() * img_draw.cols, rect.getBottomBorder() * img_draw.rows),
														 SH::makeColor(partsList[0].toInt()), SH::makeColor(partsList[1].toInt()), 2);
				}
			} else { // Draw bounding box
				cv::rectangle(img_draw,
											cv::Point(rect.getLeftBorder() * img_draw.cols, rect.getTopBorder() * img_draw.rows),
											cv::Point(rect.getRightBorder() * img_draw.cols, rect.getBottomBorder() * img_draw.rows),
											SH::makeColor(rect.className));
			}
		}
	}
}



template<typename T>

int getMaxAndPosition( std::vector<T> input, T& Max) {
	int index = 0;
	Max = input[0];
	int position = 0;
	while (index < input.size()) {
		if (Max >= input[index]) {
			index = index + 1;
		}
		else {
			Max = input[index];
			position = index;
			index = index + 1;
		}
	}
	//std::cout << "Max is "<< Max << std::endl;
	//std::cout << "Position is at " << position << std::endl;
	return position;
}



//Sort the detections into different Objects by using 3 function.


void sortByObjectIntoMap(std::vector<SH::DetectionRectangle> detections, std::map<QString, std::vector<SH::DetectionRectangle>> &outputMap, float threshold, int flagOfNMS, float k1) {
	std::map<QString, std::vector<SH::DetectionRectangle>> mapDetect;
	std::map<QString, std::vector<SH::DetectionRectangle>>::iterator it;

	it = mapDetect.begin();

	for (int i = 0; i < detections.size(); i++) {

		QString Name = detections[i].className;
		QString test = "test";

		it = mapDetect.find(Name);

		if (it == mapDetect.end()) {//ClassName not found. 
			//Question: how to dynamically initializ a vector?
			std::string className = Name.toStdString(); //From Qstring to string, just for vector initialization.
														//std::string vectorName = "BeforeVector" + className;
														//std::vector<SH::DetectionRectangle> vectorName;//I'm not sure, if I can initialize a vector in this way.
			std::vector<SH::DetectionRectangle> detectionsCurrentClass;
			detectionsCurrentClass.push_back(detections[i]);
			mapDetect.insert(std::pair<QString, std::vector<SH::DetectionRectangle>>(detections[i].className, detectionsCurrentClass));
			//Insert new Key-Value, which is Classname-Vector of Detections.

		}
		else {

			//ClassName found. Add this detection into vector of DetectionRectangle.

			mapDetect[Name].push_back(detections[i]);

		}

	}



	//Now, all of the detection are already sorted into different vectors by their classname.

	//But in every vector, the detections could come from different objects, e.g. two left legs.



	//The map is like map<SingleObject, std::vector<SH::DetectionRectangle>> mapDetect.

	//E.g. map<LeftLeg, vector<detection> detections>

	//     map<LeftArm, vector<detection> detections>

	//So right now, we should use the function "isSameObj" to seperate them into different groups.



	std::vector<SH::DetectionRectangle> Vect;

	for (it = mapDetect.begin(); it != mapDetect.end(); it++) {

		QString NameOfClass = it->first;

		Vect = it->second;

		int k = 0;

		while (Vect.size() != 0) {
			k = k + 1;
			// Get all Probs of detections from current vector and take the biggest one and its position.
			std::vector<float> vectorProb;
			for (int i = 0; i < Vect.size(); i++) {
				vectorProb.push_back(Vect[i].prob);
			}
			int position;
			float getMax = 0.0;
			position = getMaxAndPosition(vectorProb, getMax);
			std::string strNameOfClass = NameOfClass.toStdString();
			std::string str = std::to_string(k);
			std::vector<SH::DetectionRectangle> vecSortByObj;//I'm not sure, if I can initialize a vector in this way.

			QString Qstr = QString::number(k);
			auto newKeyName = NameOfClass + Qstr;
			//outputMap.insert(std::pair<QString, std::vector<SH::DetectionRectangle>>(newKeyName, vecSortByObj));

			vecSortByObj.push_back(Vect[position]);//move the biggest into a new vector.
			std::vector<SH::DetectionRectangle>::iterator itr = (Vect.begin() + position);
			Vect.erase(itr++);// And then delete it.
			for (int i = 0; i < Vect.size(); i++) {
				bool SameObj = SH::isSameObj(Vect[i], vecSortByObj[0], threshold, flagOfNMS, k1);
				if (SameObj) {
					// If they belong to the same Object, then put them into a same vector.
					vecSortByObj.push_back(Vect[i]);
					// And delete it from the original vector.
					std::vector<SH::DetectionRectangle>::iterator newitr = (Vect.begin() + i);
					Vect.erase(newitr);
					i = i - 1;
				}
				else {
					continue;
				}

			}
			outputMap.insert(std::pair<QString, std::vector<SH::DetectionRectangle>>(newKeyName, vecSortByObj));


		}



		//All Objects, which belongs to current class, are already sorted into std::vector<SH::DetectionRectangle> vecSortByObj and added into map.

		//Then delete this class from the original map.

		//mapDetect.erase(it++);
		//std::cout << "Now map size is: " << mapDetect.size() << std::endl;

		// A safe way to erase!!

		//Move to the next class

	}



	//Now the map is like map<SingleObject, std::vector<SH::DetectionRectangle>> mapDetect.

	//E.g. <LeftArm1, vector<detection> detections>

	//     <LeftArm2, vector<detection> detections>
	// Below testing
	//std::map<QString, std::vector<SH::DetectionRectangle>>::iterator ittest;

	//ittest = mapDetect.begin();
	//for (ittest = mapDetect.begin(); ittest != mapDetect.end();ittest++) {
	//	std::cout << "Key: " << (ittest->first).toStdString() << " Value: " << ((ittest->second)[0].className).toStdString() << std::endl;
	//}

	std::cout << "SortByObjectIntoMap Finished" << std::endl;
}


void oldAdaptionBBox(std::vector<SH::DetectionRectangle> Inputs, SH::DetectionRectangle& Output) {
	std::cout << "The Input of function AdaptionBBox is " << Inputs.size() << std::endl;
	if (Inputs.size() == 0) {
		std::cout << "Error in AdaptionBBox, input size = 0" << std::endl;
	}
		std::vector<float> vectorProb;
		for (int i = 0; i < Inputs.size(); i++) {
			vectorProb.push_back(Inputs[i].prob);
		}
		int position = 0;
		float getMax = 0;
		position = getMaxAndPosition(vectorProb, getMax);
		float maxProb = getMax;
		float sumX = 0;
		float sumY = 0;
		float sumWidth = 0;
		float sumHeight = 0;
		QString className = Inputs[0].className;


		for (int i = 0; i < Inputs.size(); i++) {
			sumX = sumX + Inputs[i].x_c;
			sumY = sumY + Inputs[i].y_c;
			sumWidth = sumWidth + Inputs[i].width;
			sumHeight = sumHeight + Inputs[i].height;
			if (Inputs[i].className != className) {
				std::cout << "Error in AdaptionBBox, classname are different" << std::endl;
			}
		}
		int InputSize = Inputs.size();
		Output.x_c = sumX / InputSize;
		Output.y_c = sumY / InputSize;
		Output.width = sumWidth / InputSize;
		Output.height = sumHeight / InputSize;
		Output.prob = maxProb;
		Output.className = className;


	std::cout << "AdaptionBBox Finished" << std::endl;

}


void AdaptionBBox(std::vector<SH::DetectionRectangle> Inputs, SH::DetectionRectangle& Output,const int flag) {
	if (Inputs.size() == 0) {
		std::cout << "Error in AdaptionBBox, input size = 0" << std::endl;
	}
	if (flag == 1) { //Average Adaption	
		std::vector<float> vectorProb;
		for (int i = 0; i < Inputs.size(); i++) {
			vectorProb.push_back(Inputs[i].prob);
		}
		int position = 0;
		float getMax = 0;
		position = getMaxAndPosition(vectorProb, getMax);
		float maxProb = getMax;
		float sumX = 0;
		float sumY = 0;
		float sumWidth = 0;
		float sumHeight = 0;
		QString className = Inputs[0].className;


		for (int i = 0; i < Inputs.size(); i++) {
			sumX = sumX + Inputs[i].x_c;
			sumY = sumY + Inputs[i].y_c;
			sumWidth = sumWidth + Inputs[i].width;
			sumHeight = sumHeight + Inputs[i].height;
			if (Inputs[i].className != className) {
				std::cout << "Error in AdaptionBBox, classname are different" << std::endl;
			}
		}
		int InputSize = Inputs.size();
		Output.x_c = sumX / InputSize;
		Output.y_c = sumY / InputSize;
		Output.width = sumWidth / InputSize;
		Output.height = sumHeight / InputSize;
		Output.prob = maxProb;
		Output.className = className;
	}

	else if (flag == 2) { //Intersection based Adaption
		std::vector<float> vectorProb;
		for (int i = 0; i < Inputs.size(); i++) {
			vectorProb.push_back(Inputs[i].prob);
		}
		int position = 0;
		float getMax = 0;
		position = getMaxAndPosition(vectorProb, getMax);
		Output = Inputs[position];
		for (int i = 0; i < Inputs.size(); i++) {
			if (i == position) {
				continue;
			}
			float interSec = SH::intersectionArea(Output, Inputs[i]);
			float Area = Output.getArea(); // The area of rectangle, which has the maxProb.
			if (interSec == 0) {
				//Maybe the rectangle Inputs[i] is not a correct detection.
				std::cout << "intersection = 0, "<< Output.className.toStdString() << std::endl;

				continue;
			}
			if ((interSec / Area) <= (Inputs[i].prob / getMax)) {
				//change the parameter of Outpus.
				//float diffProb = getMax - Inputs[i].prob;
				float diffProb = Inputs[i].prob;
				float delteX = (Output.x_c - Inputs[i].x_c) * diffProb;
				Output.x_c = Output.x_c - delteX;

				float delteY = (Output.y_c - Inputs[i].y_c) * diffProb;
				Output.y_c = Output.y_c - delteY;

				float delteW = (Output.width - Inputs[i].width) * diffProb;
				Output.width = Output.width - delteW;

				float delteH = (Output.height - Inputs[i].height) * diffProb;
				Output.height = Output.height - delteH;
			}
			else {
				// this means that the current rectangle doesn't have too much influence on the Output.
			}

		}

	}
	else {	
			std::vector<float> vectorProb;
			for (int i = 0; i < Inputs.size(); i++) {
				vectorProb.push_back(Inputs[i].prob);
			}
			int position = 0;
			float getMax = 0;
			position = getMaxAndPosition(vectorProb, getMax);
			std::cout << "getMax and Position Finished" << std::endl;
			float maxProb = getMax;
			float sumX = 0;
			float sumY = 0;
			float sumWidth = 0;
			float sumHeight = 0;
			QString className = Inputs[0].className;

			float diffProb = 0;
			float currProb = 0;
			int countSize = 0;
			int count = 0;
			for (int i = 0; i < Inputs.size(); i++) {
				currProb = Inputs[i].prob;
				//diffProb = sqrt(maxProb) - sqrt(currProb);
				//diffProb = pow(maxProb, 2) - pow(currProb, 2);
				diffProb = maxProb - currProb;
				if (diffProb == 0) {
					diffProb = 1;
				}
				sumX = sumX + Inputs[i].x_c *(1 - diffProb);
				sumY = sumY + Inputs[i].y_c * (1 - diffProb);
				sumWidth = sumWidth + Inputs[i].width * (1 - diffProb);
				sumHeight = sumHeight + Inputs[i].height * (1 - diffProb);
				if (Inputs[i].className != className) {
					std::cout << "Error in AdaptionBBox, classname are different" << std::endl;
				}
				countSize = countSize + diffProb;

			}

			int InputSize = Inputs.size();
			Output.x_c = sumX / countSize;
			Output.y_c = sumY / countSize;
			Output.width = sumWidth / countSize;
			Output.height = sumHeight / countSize;
			Output.prob = maxProb;
			Output.className = className;

		}

	std::cout << "AdaptionBBox Finished" << std::endl;

}



void convertMapIntoVector(std::map<QString, std::vector<SH::DetectionRectangle>> mapDetect, std::vector<SH::DetectionRectangle>& detections,const int flag) {

	//std::vector<SH::DetectionRectangle> detections;

	std::map<QString, std::vector<SH::DetectionRectangle>>::iterator it;

	SH::DetectionRectangle detection;
	std::cout << "The Input of function convertMapIntoVector is " << mapDetect.size() << std::endl;
	std::vector<SH::DetectionRectangle> Vect;
	it = mapDetect.begin();
	auto vecc = it->second;
	std::cout << "size of vecc  is " << vecc.size() << std::endl;
	for (it = mapDetect.begin(); it != mapDetect.end(); it++) {
		Vect = it->second;
		std::cout << "Vector size is " << Vect.size() << std::endl;
		AdaptionBBox(Vect, detection, flag);
		//oldAdaptionBBox(Vect, detection);
		detections.push_back(detection);
	}

	std::cout << "convertMapIntoVector Finished" << std::endl;

}



void MergeBBox(std::vector<SH::DetectionRectangle> inputs, std::vector<SH::DetectionRectangle> &outputs, const float threshold, const int flag, const int flagOfNMS, float k1) {

	std::map<QString, std::vector<SH::DetectionRectangle>> mapDetect;
	sortByObjectIntoMap(inputs, mapDetect, threshold, flagOfNMS, k1);

	//std::cout <<"The Input of function sortByObjectIntoMap is: "<< inputs.size() << std::endl;
	//std::cout << "The Output of function sortByObjectIntoMap, mapDetect size is: " << mapDetect.size() << std::endl;

	//std::cout << "The Input of function convertMapIntoVector is: " << mapDetect.size() << std::endl;

	convertMapIntoVector(mapDetect, outputs, flag);
	std::cout << "The Output of function convertMapIntoVector is " << outputs.size() << std::endl;
	std::cout << "MergeBBox Finished" << std::endl;



}

float evaluate(std::vector<SH::DetectionRectangle> detectionsGT, std::vector<SH::DetectionRectangle> detections) {
	int sizeDetections = detections.size();
	int sizeDetectionsGT = detectionsGT.size();
	int trueDetect = 0;
	for (int i = 0; i < sizeDetections; i++) {
		auto detection = detections[i];
		for (int j = 0; j < sizeDetectionsGT; j++) {
			auto detectionGT = detectionsGT[j];
			if (detectionGT.className == detection.className) {
				if (SH::isSameObj(detection, detectionGT, 0.1, 3, 1.35)) {
					trueDetect = trueDetect + 1;
				}
			}
		}
	}
	float acc = ((float)trueDetect / (float)sizeDetectionsGT);
	std::cout << "acc is " << acc << std::endl;
	return acc;
}


enum Keypoint_Type {
	nose,
	left_eye,
	right_eye,
	left_ear,
	right_ear,
	left_shoulder,
	right_shoulder,
	left_elbow,
	right_elbow,
	left_wrist,
	right_wrist,
	left_hip,
	right_hip,
	left_knee,
	right_knee,
	left_ankle,
	right_ankle
};


struct Keypoint {
	float x;
	float y;
	bool available = 0;
	Keypoint_Type type;
};

struct Person {

	Person() {
		for (int idx_keypoint = 0; idx_keypoint < 17; idx_keypoint++) {
			Keypoint keypoint;
			keypoint.type = (Keypoint_Type)idx_keypoint;
			keypoints.push_back(keypoint);
		}
	}

	std::vector<Keypoint> keypoints;
};

void slashConvert(std::vector<SH::DetectionRectangle> detections, std::vector<SH::DetectionRectangle>& output) { // convert keypoints_pair from "/" or "\" into "-"
	
	//"7/9" w+, h+
	//"9\7" w-, h+
	//"9/7" w-, h-
	//"7\9" w+, h-

	for (int i = 0; i < detections.size(); i++) {
		QString ClassName = detections[i].className;
		float left = detections[i].getLeftBorder();
		float right = detections[i].getRightBorder();
		float top = detections[i].getTopBorder();
		float bottom = detections[i].getBottomBorder();
		SH::DetectionRectangle newDetection;
		newDetection.prob = detections[i].prob;

		if (ClassName.contains("/")) {
			QStringList partsList = ClassName.split("/");
			if (partsList.size() == 2) {
				QString part1 = partsList[0];
				int idx1 = part1.toInt();
				QString part2 = partsList[1];
				int idx2 = part2.toInt();

				if (idx1 < idx2) {
					newDetection.width = detections[i].width;
					newDetection.height = -1 *detections[i].height;
					newDetection.x_c = (left + right) / 2;
					newDetection.y_c = (top + bottom) / 2;
					QString newClassName;
					newClassName = part1 + "-" + part2;
					newDetection.className = newClassName;

					output.push_back(newDetection);
				}
				else {
					newDetection.width = -1 * detections[i].width;
					newDetection.height = detections[i].height;
					newDetection.x_c = (left + right) / 2;
					newDetection.y_c = (top + bottom) / 2;
					QString newClassName;
					newClassName = part2 + "-" + part1;
					newDetection.className = newClassName;

					output.push_back(newDetection);

				}
			}

		}
		else if(ClassName.contains("\\")){
			QStringList partsList = ClassName.split("\\");
			if (partsList.size() == 2) {
				QString part1 = partsList[0];
				int idx1 = part1.toInt();
				QString part2 = partsList[1];
				int idx2 = part2.toInt();

				if (idx1 < idx2) {
					newDetection.width = detections[i].width;
					newDetection.height = detections[i].height;
					newDetection.x_c = (left + right) / 2;
					newDetection.y_c = (top + bottom) / 2;
					QString newClassName;
					newClassName = part1 + "-" + part2;
					newDetection.className = newClassName;

					output.push_back(newDetection);
				}
				else {
					newDetection.width = -1 * detections[i].width;
					newDetection.height = -1 *detections[i].height;
					newDetection.x_c = (left + right) / 2;
					newDetection.y_c = (top + bottom) / 2;
					QString newClassName;
					newClassName = part2 + "-" + part1;
					newDetection.className = newClassName;

					output.push_back(newDetection);

				}
			}
		}
		else
		{
			output.push_back(detections[i]);
		}
	}
}
//void person_detection_tmp(QString ZBSAnnotationsFilePath, std::vector<SH::DetectionRectangle>& output, const float threshold, const int flag, const int flagOfNMS, float k1) {
//	std::vector<SH::DetectionRectangle> detections; // = readZBSAnnotationsFile(...)
//	detections = readZBSAnnotationsFile(ZBSAnnotationsFilePath);
//	// NMS-Suppression
//	std::vector<SH::DetectionRectangle> NMSdetections;
//	MergeBBox(detections, NMSdetections, threshold, flag, flagOfNMS, k1);
//	std::vector<SH::DetectionRectangle> person_detections; // filter for class name "person"
//	for (int i = 0; i < NMSdetections.size(); i++) {
//		QString ClassName = NMSdetections[i].className;
//		QString ClassPerson = "person";
//		if (ClassName.compare(ClassPerson) == 0) {
//			person_detections.push_back(NMSdetections[i]);
//		}
//	}
//
//	for (auto& person_det : person_detections) {
//		Person person;
//	}
//
//
//	std::vector<SH::DetectionRectangle> keypoint_pair_detections; // filter for keypoint pair detections
//	for (int i = 0; i < NMSdetections.size(); i++) {
//		QString ClassName = NMSdetections[i].className;
//		if ((ClassName.contains("/")) || (ClassName.contains("\\"))) {
//			keypoint_pair_detections.push_back(NMSdetections[i]);
//		}
//	}
//
//	for (auto& person_detection : person_detections) { // keypoint_pairs den einzelnen Personen zuordnen
//		float person_left = person_detection.getLeftBorder();
//		float person_right = person_detection.getRightBorder();
//		float person_top = person_detection.getTopBorder();
//		float person_bottom = person_detection.getBottomBorder();
//		Person person_detection;
//		for (auto keypoint_pair : keypoint_pair_detections) {
//			//float pair_left = keypoint_pair.getLeftBorder();
//			//float pair_right = keypoint_pair.getRightBorder();
//			//float pair_top = keypoint_pair.getTopBorder();
//			//float pair_bottom = keypoint_pair.getBottomBorder();
//			// just use the center point of keypoint pair
//			float keypoint_pair_x_c = keypoint_pair.x_c;
//			float keypoint_pair_y_c = keypoint_pair.y_c;
//
//			if (keypoint_pair_x_c >= person_left && keypoint_pair_x_c <= person_right
//				&& keypoint_pair_y_c <= person_bottom && keypoint_pair_y_c >= person_top) {
//
//				QString ClassName = keypoint_pair.className;
//
//				if (ClassName.contains("/")) {
//					QStringList partsList = ClassName.split('/');
//					if (partsList.size() == 2) {
//						QString part1 = partsList[0];
//						int idx1 = part1.toInt();
//						QString part2 = partsList[1];
//						int idx2 = part2.toInt();
//
//						Keypoint keypoint1, keypoint2;
//						keypoint1.type = (Keypoint_Type)idx1;
//						keypoint1.x = keypoint_pair.getRightBorder();
//						keypoint1.y = keypoint_pair.getTopBorder();
//						keypoint1.available = 1;
//
//						keypoint2.type = (Keypoint_Type)idx1;
//						keypoint2.x = keypoint_pair.getLeftBorder();
//						keypoint2.y = keypoint_pair.getBottomBorder();
//						keypoint2.available = 1;
//
//						person_detection.keypoints.push_back(keypoint1);
//						person_detection.keypoints.push_back(keypoint2);
//					}
//				}
//				else if (ClassName.contains("\\")) {
//					QStringList partsList = ClassName.split('\\');
//					if (partsList.size() == 2) {
//						if (partsList.size() == 2) {
//							QString part1 = partsList[0];
//							int idx1 = part1.toInt();
//							QString part2 = partsList[1];
//							int idx2 = part2.toInt();
//
//							Keypoint keypoint1, keypoint2;
//							keypoint1.type = (Keypoint_Type)idx1;
//							keypoint1.x = keypoint_pair.getLeftBorder();
//							keypoint1.y = keypoint_pair.getTopBorder();
//							keypoint1.available = 1;
//
//							keypoint2.type = (Keypoint_Type)idx1;
//							keypoint2.x = keypoint_pair.getRightBorder();
//							keypoint2.y = keypoint_pair.getBottomBorder();
//							keypoint2.available = 1;
//
//							person_detection.keypoints.push_back(keypoint1);
//							person_detection.keypoints.push_back(keypoint2);
//						}
//					}
//				}
//			}
//		}
//
//		while (person_detection.keypoints[0].available == 0) {//delete some invalid keypoints
//			person_detection.keypoints.erase(person_detection.keypoints.begin());
//		}
//
//		// for "keypoint_pairs einer Person: Keypoint-Koordinaten zusammenfassen"
//		Person new_person_detection;
//
//		for (int i = 0; i < 17; i++) {
//			Keypoint_Type KType = (Keypoint_Type)i;
//			Keypoint currentKeypoint;
//			std::vector<Keypoint> Keypoints;
//
//			float sum_x = 0;
//			float sum_y = 0;
//			int k = 0;
//			while (typeid(KType) == typeid(person_detection.keypoints[k].type)) {
//				sum_x = sum_x + person_detection.keypoints[k].x;
//				sum_y = sum_y + person_detection.keypoints[k].y;
//				Keypoints.push_back(person_detection.keypoints[k]);
//				k = k + 1;
//				if (k >= person_detection.keypoints.size()) {
//					break;
//				}
//			}
//
//			currentKeypoint.x = sum_x / Keypoints.size();
//			currentKeypoint.y = sum_y / Keypoints.size();
//			currentKeypoint.available = 1;
//			currentKeypoint.type = KType;
//			new_person_detection.keypoints.push_back(currentKeypoint);
//		}
//		while (new_person_detection.keypoints[0].available == 0) {//delete some invalid keypoints
//			new_person_detection.keypoints.erase(new_person_detection.keypoints.begin());
//		}
//
//
//
//	}
//}

void PersonToKVerbindung(Person& person, std::vector<SH::DetectionRectangle>& output) {
	for (int i = 0; i < person.keypoints.size(); i++) {
		auto keypointBig = person.keypoints[i].type;
		if (keypointBig == (Keypoint_Type)1) {
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)0) {
					SH::DetectionRectangle detection;
					detection.className = "0-1";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)2)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)0) {
					SH::DetectionRectangle detection;
					detection.className = "0-2";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)3)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)1) {
					SH::DetectionRectangle detection;
					detection.className = "1-3";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)4)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)2) {
					SH::DetectionRectangle detection;
					detection.className = "2-4";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)5)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)3) {
					SH::DetectionRectangle detection;
					detection.className = "3-5";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)6)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)4) {
					SH::DetectionRectangle detection;
					detection.className = "4-6";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}else if(keypointSmall == (Keypoint_Type)5){
					SH::DetectionRectangle detection;
					detection.className = "5-6";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)7)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)5) {
					SH::DetectionRectangle detection;
					detection.className = "5-7";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)8)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)6) {
					SH::DetectionRectangle detection;
					detection.className = "6-8";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)9)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)7) {
					SH::DetectionRectangle detection;
					detection.className = "7-9";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)10)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)8) {
					SH::DetectionRectangle detection;
					detection.className = "8-10";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)11)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)5) {
					SH::DetectionRectangle detection;
					detection.className = "5-11";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)12)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)6) {
					SH::DetectionRectangle detection;
					detection.className = "6-12";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
				else if (keypointSmall == (Keypoint_Type)11) {
					SH::DetectionRectangle detection;
					detection.className = "11-12";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)13)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)11) {
					SH::DetectionRectangle detection;
					detection.className = "11-13";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}

		else if (keypointBig == (Keypoint_Type)14)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)12) {
					SH::DetectionRectangle detection;
					detection.className = "12-14";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)15)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)13) {
					SH::DetectionRectangle detection;
					detection.className = "13-15";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
		else if (keypointBig == (Keypoint_Type)16)
		{
			for (int k = 0; k < person.keypoints.size(); k++) {
				auto keypointSmall = person.keypoints[k].type;
				if (keypointSmall == (Keypoint_Type)14) {
					SH::DetectionRectangle detection;
					detection.className = "14-16";
					detection.x_c = (person.keypoints[i].x + person.keypoints[k].x) / 2;
					detection.y_c = (person.keypoints[i].y + person.keypoints[k].y) / 2;
					detection.width = person.keypoints[i].x - person.keypoints[k].x;
					detection.height = person.keypoints[i].y - person.keypoints[k].y;
					output.push_back(detection);
				}
			}
		}
	}
}

void person_detection_tmp(QString ZBSAnnotationsFilePath, Person& new_person_detection, const float threshold, const int flag, const int flagOfNMS, float k1) {
	std::vector<SH::DetectionRectangle> detections; // = readZBSAnnotationsFile(...)
	detections = readZBSAnnotationsFile(ZBSAnnotationsFilePath);

	std::vector<SH::DetectionRectangle> converted;
	slashConvert(detections, converted);
	// NMS-Suppression
	std::vector<SH::DetectionRectangle> NMSdetections;
	MergeBBox(converted, NMSdetections, threshold, flag, flagOfNMS, k1);
	std::vector<SH::DetectionRectangle> person_detections; // filter for class name "person"
	for (int i = 0; i < NMSdetections.size(); i++) {
		QString ClassName = NMSdetections[i].className;
		QString ClassPerson = "person";
		if (ClassName.compare(ClassPerson) == 0) {
			person_detections.push_back(NMSdetections[i]);
		}
	}


	std::vector<SH::DetectionRectangle> keypoint_pair_detections; // filter for keypoint pair detections
	for (int i = 0; i < NMSdetections.size(); i++) {
		QString ClassName = NMSdetections[i].className;
		if ((ClassName.contains("-"))) { 
			keypoint_pair_detections.push_back(NMSdetections[i]);
		}
	}

	for (auto& person_detection : person_detections) { // keypoint_pairs den einzelnen Personen zuordnen
		float person_left = person_detection.getLeftBorder();
		float person_right = person_detection.getRightBorder();
		float person_top = person_detection.getTopBorder();
		float person_bottom = person_detection.getBottomBorder();
		Person person_detection;
		for (auto keypoint_pair : keypoint_pair_detections) {
			//float pair_left = keypoint_pair.getLeftBorder();
			//float pair_right = keypoint_pair.getRightBorder();
			//float pair_top = keypoint_pair.getTopBorder();
			//float pair_bottom = keypoint_pair.getBottomBorder();
			// just use the center point of keypoint pair
			float keypoint_pair_x_c = keypoint_pair.x_c;
			float keypoint_pair_y_c = keypoint_pair.y_c;

			if (keypoint_pair_x_c >= person_left && keypoint_pair_x_c <= person_right
				&& keypoint_pair_y_c <= person_bottom && keypoint_pair_y_c >= person_top) {

				QString ClassName = keypoint_pair.className;

				if (ClassName.contains("-")) {
					QStringList partsList = ClassName.split("-");
					if (partsList.size() == 2) {
						QString part1 = partsList[0];
						int idx1 = part1.toInt();
						QString part2 = partsList[1];
						int idx2 = part2.toInt();

						Keypoint keypoint1, keypoint2;
						keypoint1.type = (Keypoint_Type)idx1;
						keypoint1.x = keypoint_pair.x_c - (keypoint_pair.width) / 2;
						keypoint1.y = keypoint_pair.y_c + (keypoint_pair.height) / 2;
						keypoint1.available = 1;

						keypoint2.type = (Keypoint_Type)idx1;
						keypoint2.x = keypoint_pair.x_c + (keypoint_pair.width) / 2;
						keypoint2.y = keypoint_pair.y_c - (keypoint_pair.height) / 2;
						keypoint2.available = 1;

						person_detection.keypoints.push_back(keypoint1);
						person_detection.keypoints.push_back(keypoint2);						
					}
				}
			}
		}

		while (person_detection.keypoints[0].available == 0) {//delete some invalid keypoints
			person_detection.keypoints.erase(person_detection.keypoints.begin() );
		}

		// for "keypoint_pairs einer Person: Keypoint-Koordinaten zusammenfassen"

		for (int i = 0; i < 17; i++) {
			Keypoint_Type KType = (Keypoint_Type)i;
			Keypoint currentKeypoint;
			std::vector<Keypoint> Keypoints;

			float sum_x = 0;
			float sum_y = 0;

			for (int k = 0; k < person_detection.keypoints.size(); k++) {
				if (KType == person_detection.keypoints[k].type) {
					sum_x = sum_x + person_detection.keypoints[k].x;
					sum_y = sum_y + person_detection.keypoints[k].y;
					Keypoints.push_back(person_detection.keypoints[k]);
				}
			}
			if (sum_x != 0) {
				currentKeypoint.x = sum_x / Keypoints.size();
				currentKeypoint.y = sum_y / Keypoints.size();
				currentKeypoint.available = 1;
				currentKeypoint.type = KType;
				new_person_detection.keypoints.push_back(currentKeypoint);
			}

		}
		while (new_person_detection.keypoints[0].available == 0) {//delete some invalid keypoints
			new_person_detection.keypoints.erase(new_person_detection.keypoints.begin());
		}



	}
}

void person_detection_GT(QString AnnotationsPathGT, Person& new_person_detection) {
	std::vector<SH::DetectionRectangle> detections; // = readZBSAnnotationsFile(...)
	detections = readZBSAnnotationsFile(AnnotationsPathGT);

	std::vector<SH::DetectionRectangle> converted;
	slashConvert(detections, converted);

	std::vector<SH::DetectionRectangle> person_detections; // filter for class name "person"
	for (int i = 0; i < converted.size(); i++) {
		QString ClassName = converted[i].className;
		QString ClassPerson = "person";
		if (ClassName.compare(ClassPerson) == 0) {
			person_detections.push_back(converted[i]);
		}
	}

	for (auto& person_det : person_detections) {
		Person person;
	}


	std::vector<SH::DetectionRectangle> keypoint_pair_detections; // filter for keypoint pair detections
	for (int i = 0; i < converted.size(); i++) {
		QString ClassName = converted[i].className;
		if ((ClassName.contains("-"))) {
			keypoint_pair_detections.push_back(converted[i]);
		}
	}

	for (auto& person_detection : person_detections) { // keypoint_pairs den einzelnen Personen zuordnen
		float person_left = person_detection.getLeftBorder();
		float person_right = person_detection.getRightBorder();
		float person_top = person_detection.getTopBorder();
		float person_bottom = person_detection.getBottomBorder();
		Person person_detection;
		for (auto keypoint_pair : keypoint_pair_detections) {
			//float pair_left = keypoint_pair.getLeftBorder();
			//float pair_right = keypoint_pair.getRightBorder();
			//float pair_top = keypoint_pair.getTopBorder();
			//float pair_bottom = keypoint_pair.getBottomBorder();
			// just use the center point of keypoint pair
			float keypoint_pair_x_c = keypoint_pair.x_c;
			float keypoint_pair_y_c = keypoint_pair.y_c;

			if (keypoint_pair_x_c >= person_left && keypoint_pair_x_c <= person_right
				&& keypoint_pair_y_c <= person_bottom && keypoint_pair_y_c >= person_top) {

				QString ClassName = keypoint_pair.className;

				if (ClassName.contains("-")) {
					QStringList partsList = ClassName.split("-");
					if (partsList.size() == 2) {
						QString part1 = partsList[0];
						int idx1 = part1.toInt();
						QString part2 = partsList[1];
						int idx2 = part2.toInt();

						Keypoint keypoint1, keypoint2;
						keypoint1.type = (Keypoint_Type)idx1;
						keypoint1.x = keypoint_pair.x_c - (keypoint_pair.width) / 2;
						keypoint1.y = keypoint_pair.y_c + (keypoint_pair.height) / 2;
						keypoint1.available = 1;

						keypoint2.type = (Keypoint_Type)idx1;
						keypoint2.x = keypoint_pair.x_c + (keypoint_pair.width) / 2;
						keypoint2.y = keypoint_pair.y_c - (keypoint_pair.height) / 2;
						keypoint2.available = 1;

						person_detection.keypoints.push_back(keypoint1);
						person_detection.keypoints.push_back(keypoint2);
					}
				}
			}
		}

		while (person_detection.keypoints[0].available == 0) {//delete some invalid keypoints
			person_detection.keypoints.erase(person_detection.keypoints.begin());
		}

		// for "keypoint_pairs einer Person: Keypoint-Koordinaten zusammenfassen"

		for (int i = 0; i < 17; i++) {
			Keypoint_Type KType = (Keypoint_Type)i;
			Keypoint currentKeypoint;
			std::vector<Keypoint> Keypoints;

			float sum_x = 0;
			float sum_y = 0;

			for (int k = 0; k < person_detection.keypoints.size(); k++) {
				if (KType == person_detection.keypoints[k].type) {
					sum_x = sum_x + person_detection.keypoints[k].x;
					sum_y = sum_y + person_detection.keypoints[k].y;
					Keypoints.push_back(person_detection.keypoints[k]);
				}
			}
			if (sum_x != 0) {
				currentKeypoint.x = sum_x / Keypoints.size();
				currentKeypoint.y = sum_y / Keypoints.size();
				currentKeypoint.available = 1;
				currentKeypoint.type = KType;
				new_person_detection.keypoints.push_back(currentKeypoint);
			}
		}
		while (new_person_detection.keypoints[0].available == 0) {//delete some invalid keypoints
			new_person_detection.keypoints.erase(new_person_detection.keypoints.begin());
		}



	}
}


float evaluationKeypoint(Person Detected, Person GTPerson) {
	float sum = 0;
	int found = 0;
	for (int i = 0; i < Detected.keypoints.size(); i++) {
		Keypoint_Type type = Detected.keypoints[i].type;
		for (int k = 0; k < GTPerson.keypoints.size(); k++) {
			Keypoint_Type GTtype = GTPerson.keypoints[k].type;
			if (type == GTtype) {
				float distance = sqrt(pow((Detected.keypoints[i].x - GTPerson.keypoints[k].x), 2.0) + pow((Detected.keypoints[i].y - GTPerson.keypoints[k].y), 2.0));
				sum = sum + distance;
				found = found + 1;
			}
		}
	}

	return sum / found;
}

float evalationObj(std::vector<SH::DetectionRectangle>&detections, std::vector<SH::DetectionRectangle>& GT) {
	int detected = 0; //number of detected objects
	float sumIoU = 0;// sum of IoU 
	for (int i = 0; i < GT.size(); i++) {
		auto detectionGT = GT[i];
		QString ClassName = detectionGT.className;
		for (int k = 0; k < detections.size(); k++) {
			auto detection = detections[k];
			if (ClassName.compare(detection.className) == 0) {
				float curIoU = SH::intersectionOverUnion(detection, detectionGT);
				if (curIoU >= 0.1) {
					detected = detected + 1;
				}
				sumIoU = sumIoU + curIoU;
			}
		}
	}

	float curracc = (float)detected / (float)GT.size();
	float avgIoU = sumIoU / GT.size();
	return curracc;

}

float evaluation(QString AnnotationsPathGT, QString AnnotationsPath, const float isSameObjthreshold, const int AdaptionBoxflag, const int flagOfNMS, float k1) {
	Person person, personGT;
	person_detection_GT(AnnotationsPathGT, personGT);
	person_detection_tmp(AnnotationsPath, person, isSameObjthreshold, AdaptionBoxflag, flagOfNMS, k1);
	std::vector<SH::DetectionRectangle> detectionPerson, detectionPersonGT;
	PersonToKVerbindung(person, detectionPerson);
	PersonToKVerbindung(personGT, detectionPersonGT);
	float acc;
	acc = evalationObj(detectionPerson, detectionPersonGT);

	return acc;

}

float dirEvaluation(QString dirpath, QString dirpathGT, const float isSameObjthreshold, const int AdaptionBoxflag, const int flagOfNMS, float k1) {

	QDir dir(dirpath);
	QStringList nameFilters;
	nameFilters << "*.annotations";
	QStringList files = dir.entryList(nameFilters, QDir::Files | QDir::Readable, QDir::Name);


	QDir dirGT(dirpathGT);
	QStringList nameFiltersGT;
	nameFiltersGT << "*.annotations";
	QStringList filesGT = dirGT.entryList(nameFiltersGT, QDir::Files | QDir::Readable, QDir::Name);
	float sumAcc = 0;
	for (int i = 0; i < files.size(); i++) {
		QString Anno = files[i];
		QString AnnoGT = filesGT[i];
		Anno = dirpath + Anno;
		AnnoGT = dirpathGT + AnnoGT;
		float currentAcc = evaluation(AnnoGT, Anno, isSameObjthreshold, AdaptionBoxflag, flagOfNMS, k1);
		sumAcc = sumAcc + currentAcc;
		//std::cout << "current sumAcc is" << sumAcc << std::endl;
		//std::cout << "current i is" << i << std::endl;

	}

	return (sumAcc / (float)files.size());

}


int main() {
	std::string ImgPath = "C:/Data/NMS_test/000000000036.jpg";
	QString AnnotationsPath ="C:/Data/NMS_test/000000000036.annotations" ;
	QString AnnotationsPathGT = "C:/Data/NMS_test/000000000036GT.annotations";

	QString dirpathGT = "C:/Data/NMS_test/AnnoGT/";
	QString dirpath = "C:/Data/NMS_test/Anno/";
	float max = 0;
	float maxk1 = 0.1;
	for (float k1 = 0.1; k1 <= 1; k1 = k1+ 0.1) {
		float test = 0;
		test = dirEvaluation(dirpath, dirpathGT, 1, 2, 3,k1);
		if (test > max) {
			max = test;
			maxk1 = k1;
		}
	}


	//File:000000000785, 000000004395,000000006471,000000000063
	// Example 1: Load Annotations
	//auto detections = readZBSAnnotationsFile("C:/Data/CocoAnnotations/test2017/000000000016.annotations"); //@Zhe: Path needs to be adjusted
	auto detections = readZBSAnnotationsFile(AnnotationsPath);
	auto detectionGT = readZBSAnnotationsFile(AnnotationsPathGT);
	std::vector<SH::DetectionRectangle> newdetections, comparedetections;
	// Example 2: Write Annotations
	//writeZBSAnnotationsFile(detections, "D:/test.annotations"); //@Zhe: Path needs to be adjusted
	int flag = 2;
	//float NMS_Threshlod = 0.1;
	float NMS_Threshlod = 0.1;
	int flagOfNMS = 3;
	float k1 = 0.35;
	//float k1 = 1.35;
	MergeBBox(detections, newdetections, NMS_Threshlod, flag, flagOfNMS, k1);
	MergeBBox(detections, comparedetections, NMS_Threshlod, 1, flagOfNMS, k1);
	float acc = 0;
	acc = evaluate(detectionGT, newdetections);
	std::cout << "acc is " << acc << std::endl;

	// Example 3: Show image with annotations
	//cv::Mat image = cv::imread("C:/Data/CocoAnnotations/test2017/000000000016.jpg"); //@Zhe: Path needs to be adjusted
	writeZBSAnnotationsFile(newdetections, "C:/Data/NMS_test/test.annotations"); //@Zhe: Path needs to be adjusted

	cv::Mat image = cv::imread(ImgPath); 
	cv::Mat compareimage = cv::imread(ImgPath);
	cv::Mat newimage = cv::imread(ImgPath);

	drawKeypoints(image, detections);
	//drawKeypoints(compareimage, comparedetections);
	drawKeypoints(compareimage, detectionGT);
	drawKeypoints(newimage, newdetections);


	cv::Mat honcat, output;
	cv::hconcat(image, compareimage, output);
	cv::hconcat(output, newimage, honcat);

	cv::imshow("honcat", honcat);

	cv::waitKey(0);
}
