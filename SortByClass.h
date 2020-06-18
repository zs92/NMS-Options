#include <map>;
#include <QtCore>
#include <QString>
#include <iostream>

#include "SHRectangle.h"
#include "SHColor.h"

std::map<QString, std::vector<SH::DetectionRectangle>> mapDetect;

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
	std::cout << "Max is "<< Max << std::endl;
	std::cout << "Position is at " << position << std::endl;
	return position;
}



//Sort the detections into different Objects by using isSameObj function.


void sortByObjectIntoMap(std::vector<SH::DetectionRectangle> detections, std::map<QString, std::vector<SH::DetectionRectangle>> &outputMap, float threshold) {
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
			std::cout << "Insert new Classname-Vector Finished" << std::endl;

		}
		else {

			//ClassName found. Add this detection into vector of DetectionRectangle.

			mapDetect[Name].push_back(detections[i]);
			std::cout << "ClassName found in sortByObjectintoMap Finished" << std::endl;

		}

	}



	//Now, all of the detection are already sorted into different vectors by their classname.

	//But in every vector, the detections could come from different objects, e.g. two left legs.



	//The map is like map<SingleObject, std::vector<SH::DetectionRectangle>> mapDetect.

	//E.g. map<LeftLeg, vector<detection> detections>

	//     map<LeftArm, vector<detection> detections>

	//So right now, we should use the function "isSameObj" to seperate them into different groups.

	std::cout << "Now map size is: " << mapDetect.size() << std::endl;


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
				std::cout << "Prob push_back to vectorProb Finished" << std::endl;
			}
			int position;
			float getMax = 0.0;
			position = getMaxAndPosition(vectorProb, getMax);
			std::cout << "getMax and Position Finished" << std::endl;
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
				bool SameObj = SH::isSameObj(Vect[i], vecSortByObj[0], threshold);
				if (SameObj) {
					// If they belong to the same Object, then put them into a same vector.
					vecSortByObj.push_back(Vect[i]);
					// And delete it from the original vector.
					std::vector<SH::DetectionRectangle>::iterator newitr = (Vect.begin() + i);
					Vect.erase(newitr);
					i = i - 1;
					std::cout << "Vec erase Finished" << std::endl;
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

		std::cout << "mapDetect erase Finished" << std::endl;
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
	std::cout << "At the end of sortByObjectIntoMap, map size is: " << mapDetect.size() << std::endl;

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
		std::cout << "getMax and Position Finished" << std::endl;
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
	std::cout << "The Input of function AdaptionBBox is " << Inputs.size() << std::endl;
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
		std::cout << "getMax and Position Finished" << std::endl;
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
		std::cout << "getMax and Position Finished" << std::endl;
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

				//continue;
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



void MergeBBox(std::vector<SH::DetectionRectangle> inputs, std::vector<SH::DetectionRectangle> &outputs, const float threshold, const int flag) {

	std::map<QString, std::vector<SH::DetectionRectangle>> mapDetect;
	sortByObjectIntoMap(inputs, mapDetect, threshold);

	//std::cout <<"The Input of function sortByObjectIntoMap is: "<< inputs.size() << std::endl;
	//std::cout << "The Output of function sortByObjectIntoMap, mapDetect size is: " << mapDetect.size() << std::endl;

	//std::cout << "The Input of function convertMapIntoVector is: " << mapDetect.size() << std::endl;

	convertMapIntoVector(mapDetect, outputs, flag);
	std::cout << "The Output of function convertMapIntoVector is " << outputs.size() << std::endl;
	std::cout << "MergeBBox Finished" << std::endl;



}
