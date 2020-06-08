#include <map>;
#include <QtCore>
#include <QString>

#include "SHRectangle.h"
#include "SHColor.h"

std::map<QString, std::vector<SH::DetectionRectangle>> mapDetect;

//Sort the detections into different Objects by using isSameObj function.
void sortByClass(std::map<QString, std::vector<SH::DetectionRectangle>> mapDetect, std::vector<SH::DetectionRectangle> detections){
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

	//Now, all of the detection are already sorted into different vector by their classname.
	//But in every vector, the detections could come from different objects, e.g. two left legs.
	//So right now, we should use the function "isSameObj" to seperate them into different groups.
	for(it = mapDetect.begin(); it != mapDetect.end(); it++){
		NameOfClass = it->first;
		Vect = it->second;
	}

}



