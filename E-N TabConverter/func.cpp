#pragma once
#include "Dodo.h"
#include "global.h"
#include "Cuckoo.h"
#include "opencv.hpp"
#include "swan.h"
#include "tools.h"

using namespace std;

#define imdebug(img, title) imshow((img), (title)); cv::waitKey()

GlobalPool *global = NULL;

int TrainMode() {
	train();
	return 0;
}

extern notify<int> progress;
extern notify<string> notification;

int go(string f,bool isCut) {
	bool flag = false;
	std::vector<space> coll;
	cv::Mat img = threshold(f);
	if (img.empty()) {
		err ex = { 3,__LINE__,"Wrong format." };
		throw ex;
	}
	
	cv::Mat trimmed = trim(img);
	float screenCols = 1919 / 1.25f;				//1919, 最大显示宽度；1.25， win10 系统缩放比
													//TODO：不知道怎么获取QAQ
	if (trimmed.cols > screenCols) {
		float toRows = screenCols / trimmed.cols * trimmed.rows;
		trimmed = perspect(trimmed, screenCols, (int) toRows);
	}
	std::vector<cv::Mat> piece;
	//imdebug("trimmed pic", trimmed);
	
	splitter piccut(trimmed);
	piccut.start(piece);
	
	progress = 1;
	notification = "过滤算法正常";
	
	global = new GlobalPool(cfgPath,trimmed.cols);
	std::vector<measure> sections;
	std::vector<cv::Mat> timeValue;
	std::vector<cv::Mat> info;
	std::vector<cv::Mat> notes;
	std::vector<cv::Vec4i> pos;
	std::vector<cv::Mat> nums;
	cv::Mat toOCR;

	int k = 1;
	size_t n = piece.size();

	for (int i = 0; i < n; i++) {
		if (piece[i].empty()) continue;
		std::vector<cv::Vec4i> rows;
		vector<int> thick;
		findRow(piece[i],toOCR, CV_PI / 18, rows,thick);
		if (rows.size() == 6) {
			flag = true;

			vector<cv::Vec4i> lines;
			int max = min(rows[5][1], rows[5][3]);
			int min = std::max(rows[0][1], rows[0][3]);

			findCol(piece[i], CV_PI / 18 * 8, max, min, thick, lines);
			std::vector<cv::Mat> origin;
			std::vector<cv::Mat> section;
			if (lines.size()) {
				cut(toOCR, lines, 0, section, true);
				cut(piece[i], lines, 0, origin, true);
			}
			global->rowLenth += piece[i].rows;
			for (size_t j = 0; j < section.size(); j++) {
				try {
					measure newSec(origin[j], section[j], rows, (int)k);
					if (SUCCEED(newSec.id)) k++;
					sections.emplace_back(newSec);
				}
				catch (err ex) {
					switch (ex.id){
					case 1:
					default: throw ex; break;
					}
				}
			}
		}
		else {
			if(flag) notes.emplace_back(piece[i]);
			else info.emplace_back(piece[i]);
		}
		progress = progress + 49 / (int)n;
	}
	piece.clear();

	notification = "扫描完成，准备写入文件";
	progress = 50;
	delete global;
	char name[256] = "";
	fname(f.c_str(),name);
	saveDoc finish(name,"unknown","unknown","unknown","E-N TabConverter","Internet");
	for (measure& i : sections) {
		if(SUCCEED(i.id))
		try { 
			finish.saveMeasure(i); 
		}
		catch (err ex) {
			switch (ex.id)
			{
			case 3: break;				//unexpected timeValue
										//impossible
			default: break;
			}
		}
		progress = progress + 50 / (int)sections.size();
	}
	string fn = name;
	fn = fn + ".xml";
	finish.save(fn.c_str());
	progress = 100;
	destroyAllWindows();
	return 0;
}