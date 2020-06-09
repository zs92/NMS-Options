#include <map>;
#include <QtCore>
#include <QString>
#include <iostream>

#include "SHRectangle.h"
#include "SHColor.h"

std::map<QString, std::vector<SH::DetectionRectangle>> mapDetect;

//Sort the detections into different Objects by using isSameObj function.
void sortByObjectIntoMap(std::vector<SH::DetectionRectangle> detections, std::map<QString, std::vector<SH::DetectionRectangle>> mapDetect, float threshold){
	std::map<QString, std::vector<SH::DetectionRectangle>>::iterator it;
	it = mapDetect.begin();
	for(int i = 0; i < detections.size(); i++){
		Name = detections[i].className;
		it = mapDetect.find(Name);
		if(it == mapDetect.end()){//ClassName not found. 

			className = Name.toStdString(); //From Qstring to string, just for vector initialization.
			std::string vectorName = "BeforeVector"+className;
			std::vector<SH::DetectionRectangle> vectorName;//I'm not sure, if I can initialize a vector in this way.

			mapDetect.insert(std::pair<QString,std::vector<SH::DetectionRectangle>>(detections[i].className, std::vector<SH::DetectionRectangle> vectorName));
			//Insert new Key-Value, which is Classname-Vector of Detections.
			vectorName.push_back(detections[i]);
		}else{
			//ClassName found. Add this detection into vector of DetectionRectangle.
			mapDetect[Name].push_back(detections[i]);
		}
	}

	//Now, all of the detection are already sorted into different vectors by their classname.
	//But in every vector, the detections could come from different objects, e.g. two left legs.

	//Now the map is like map<SingleObject, std::vector<SH::DetectionRectangle>> mapDetect.
	//E.g. map<LeftLeg, vector<detection> detections>
	//     map<LeftArm, vector<detection> detections>
	//So right now, we should use the function "isSameObj" to seperate them into different groups.


	for(it = mapDetect.begin(); it != mapDetect.end(); it++){
		NameOfClass = it->first;
		Vect = it->second;
		int k = 0;
		while(Vect.size() != 0){
			k = k + 1;
			// Get all Probs of detection from current vector and take the biggest and its position.
			std::vector<float> vectorProb;
			for(int i = 0; i < Vect.size(); i++){
				vectorProb.push_back(Vect[i].prob);
			}
    		std::vector<float>::iterator biggest = std::max_element(std::begin(Vect), std::end(Vect));
    		int maxPosition = max_element(Vect.begin(),Vect.end()) - Vect.begin(); 

    		strNameOfClass = NameOfClass.toStdString();
    		std::string str=to_string(k)
    		std::string vecSortByObj = strNameOfClass + str;//Just for a different Object having a different vector name.
    		std::vector<SH::DetectionRectangle> vecSortByObj;//I'm not sure, if I can initialize a vector in this way.
    		QString Qstr = QString::number(k);
    		auto newKeyName = NameOfClass + Qstr;

    		mapDetect.insert(std::pair<QString,std::vector<SH::DetectionRectangle>>(newKeyName, std::vector<SH::DetectionRectangle> vecSortByObj));	

    		vecSortByObj.push_back(Vect[maxPosition]);//move the biggest into a new vector.
    		Vect.erase(Vect[maxPosition]);// And then delete it.
    		for(int i = 0; i < Vect.size(); i++){
    			if( SH::isSameObj(Vect[i], vecSortByObj[0], threshold) ){
    				// If they belong to the same Object, then put them into a same vector.
    				vecSortByObj.push_back(Vect[i]);
    				// And delete it from the original vector.
    				Vect.erase(Vect[i]);
    			}
    		}   		
		}

		//All Objects, which belongs to current class, are already sorted into std::vector<SH::DetectionRectangle> vecSortByObj and added into map.
		//Then delete this class from the original map.
		mapDetect.erase(it);
		//Move to the next class
	}

	//Now the map is like map<SingleObject, std::vector<SH::DetectionRectangle>> mapDetect.
	//E.g. <LeftArm1, vector<detection> detections>
	//     <LeftArm2, vector<detection> detections>

}

void AdaptionBBox(std::vector<SH::DetectionRectangle> Inputs, SH::DetectionRectangle Output){
	if(Inputs.size() == 0){
		std::cout<<"Error in AdaptionBBox, input size = 0"<<std::endl;
	}else{
		//SH::DetectionRectangle Output;
		std::vector<float>::iterator biggest = std::max_element(std::begin(Inputs), std::end(Inputs));
		maxProb = *biggest;
		float sumX = 0;
		float sumY = 0;
		float sumWidth = 0;
		float sumHeight = 0;
		className = Inputs[0].className;
		for(int i = 0; i < Inputs.size(); i++){
			sumX  = sumX + Inputs[i].x_c;
			sumY  = sumY + Inputs[i].y_c;
			sumWidth  = sumWidth + Inputs[i].width;
			sumHeight  = sumHeight + Inputs[i].height;
			if(Inputs[i].className != className){
				std::cout<<"Error in AdaptionBBox, classname are different"<<std::endl;
			}

		}
		int InputSize = Inputs.size();
		Output.x_c = sumX/InputSize;
		Output.y_c = sumY/InputSize;
		Output.width = sumWidth/InputSize;
		Output.height = sumHeight/InputSize;
		Output.prob = maxProb;
		Output.className = className;
	}
}


void convertMapIntoVector(std::map<QString, std::vector<SH::DetectionRectangle>> mapDetect, std::vector<SH::DetectionRectangle> detections ){
	//std::vector<SH::DetectionRectangle> detections;
	SH::DetectionRectangle detection;
	for(it = mapDetect.begin(); it != mapDetect.end(); it++){
		Vect = it->second;
		AdaptionBBox(Vect, detection);
		detections.push_back(detection);
	}
}

void MergeBBox(std::vector<SH::DetectionRectangle> inputs, std::vector<SH::DetectionRectangle> outputs, float threshold){
	std::vector<SH::DetectionRectangle>> mapDetect;
	sortByObjectIntoMap(inputs, mapDetect,threshold);
	convertMapIntoVector(mapDetect, outputs);

}
