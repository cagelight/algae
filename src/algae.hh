#pragma once

#include <QDebug>
#include <QFileInfo>
#include <QImage>
#include <QMainWindow>

namespace AlgaeConstants {
	static constexpr int THUMBNAIL_SIZE = 128;
	static constexpr int TEST_SIZE = 64;
}

struct AlgaeImage {
	QFileInfo file;
	QPixmap thumb;
	std::vector<QRgb> test;
	bool valid = false;
	uint32_t sortValue = -1;
	
	void Initialize();
	static double Compare(AlgaeImage const & A, AlgaeImage const & B);
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
