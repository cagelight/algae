#pragma once

#include <QDebug>
#include <QFileInfo>
#include <QImage>
#include <QMainWindow>

#include <opencv2/opencv.hpp>

namespace AlgaeConstants {
	static constexpr int THUMBNAIL_SIZE = 128;
	static constexpr int TEST_SIZE = 64;
	static constexpr int HIST_BINS = 256;
}

struct AlgaeImage {
	QFileInfo file;
	QPixmap thumb;
	std::vector<uint8_t> test;
	bool valid = false;
	uint32_t sortValue = -1;
	
	cv::Mat b_hist, g_hist, r_hist;
	
	void Initialize();
	static double ComparePix(AlgaeImage const & A, AlgaeImage const & B);
	static double CompareHist(AlgaeImage const & A, AlgaeImage const & B);
};

class AlgaeCore : public QMainWindow {
	Q_OBJECT
	
	struct AlgaePairComp {
		uint32_t x, y;
		double value;
		
		auto operator <=>(AlgaePairComp const & other) {
			return other.value <=> value;
		}
	};
	
	struct AlgaeImageLink {
		AlgaeImage * img = nullptr;
		
		auto operator <=>(AlgaeImageLink const & other) {
			return img->sortValue <=> other.img->sortValue;
		}
	};
	
	QList<AlgaeImage> m_images;
	QList<AlgaeImageLink> m_imageLinks;
	QList<AlgaePairComp> m_comp;
	
public: 
	AlgaeCore();
	virtual ~AlgaeCore() = default;
};
