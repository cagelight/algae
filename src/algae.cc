#include "algae.hh"

#include <QtWidgets>

AlgaeCore::AlgaeCore() : QMainWindow() {
	
	// ================================================================
	// INITIALIZE UI
	// ================================================================
	
	//this->setMinimumSize(800, 600);
	
	QWidget * centerW = new QWidget {this};
	this->setCentralWidget(centerW);
	QGridLayout * centerL = new QGridLayout(centerW);
	centerW->setMinimumSize(800, 250);
	
	// CONTROL
	// ================
	
	QWidget * controlW = new QWidget(centerW); new QVBoxLayout(controlW);
	controlW->layout()->setContentsMargins(0, 0, 0, 0);
	
	QWidget * progBoxW = new QWidget(controlW); new QHBoxLayout(progBoxW);
	progBoxW->layout()->setContentsMargins(0, 0, 0, 0);
	QPushButton * initBut = new QPushButton(controlW);
	QPushButton * sortBut1 = new QPushButton(controlW);
	QPushButton * sortBut2 = new QPushButton(controlW);
	QPushButton * sortBut3 = new QPushButton(controlW);
	QPushButton * renameBut = new QPushButton(controlW);
	initBut->setText("Initialize");
	sortBut1->setText("Sort [Pix]");
	sortBut2->setText("Sort [Hist]");
	sortBut3->setText("Sort [Balanced]");
	sortBut1->setDisabled(true);
	sortBut2->setDisabled(true);
	sortBut3->setDisabled(true);
	renameBut->setText("Rename");
	renameBut->setDisabled(true);
	progBoxW->layout()->addWidget(initBut);
	progBoxW->layout()->addWidget(sortBut1);
	progBoxW->layout()->addWidget(sortBut2);
	progBoxW->layout()->addWidget(sortBut3);
	progBoxW->layout()->addWidget(renameBut);
	QProgressBar * progBar = new QProgressBar(controlW);
	progBoxW->layout()->addWidget(progBar);
	controlW->layout()->addWidget(progBoxW);
	
	centerL->addWidget(controlW, 1, 0);
	
	// THUMBS
	// ================
	
	QScrollArea * thumbsContW = new QScrollArea(centerW);
	QWidget * thumbsW = new QWidget(thumbsContW);
	thumbsContW->setWidget(thumbsW);
	thumbsContW->setWidgetResizable(true);
	
	QGridLayout * thumbsL = new QGridLayout(thumbsW);
	thumbsW->setLayout(thumbsL);
	thumbsW->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	
	centerL->addWidget(thumbsContW, 0, 0);
	
	// ================================================================
	// INITIALIZE DATA
	// ================================================================
	
	QList<QDir> dirs;
	
	auto args = QApplication::arguments();
	for (int i = 1; i < args.length(); i++) {
		if (!QDir{args[i]}.exists()) continue;
		dirs.append(args[i]);
	}
	
	for ( QDir dir : dirs ) {
		for (QFileInfo info : dir.entryInfoList()) {
			if (!info.isFile())
				continue;
			
			AlgaeImage & ai = m_images.emplace_back();
			ai.file = info;
		}
	}
	
	qDebug() << m_images.size() << "images in set";
	
	// ================================================================
	// PROCESS
	// ================================================================
	
	auto regenerateLinks = [this](){
		
		m_imageLinks.clear();
		for (AlgaeImage & ai : m_images) {
			auto & link = m_imageLinks.emplace_back();
			link.img = &ai;
		}
		std::sort(m_imageLinks.begin(), m_imageLinks.end());
	};
	
	auto displayImages = [=, this](){
		
		for (auto & w : thumbsW->findChildren<QWidget *>()) {
			w->deleteLater();
		}
		
		int i = 0;
		for (AlgaeImageLink & ai : m_imageLinks) {
			QLabel * imgL = new QLabel(thumbsW);
			imgL->setPixmap(ai.img->thumb);
			thumbsL->addWidget(imgL, 0, i);
			i++;
		}
	};
	
	connect(renameBut, &QPushButton::clicked, [=, this](){
		
		auto cnt = std::floor(std::log10(m_imageLinks.size())) + 1;
		
		for (int i = 0; i < m_imageLinks.size(); i++) {
			QFileInfo info = m_imageLinks[i].img->file;
			QDir dir = info.canonicalPath();
			auto src = info.fileName();
			auto dst = QString("%0_%1").arg(i + 1, cnt, 10, QChar('0')).arg(src);
			QFile file = info.canonicalFilePath();
			file.rename(dir.filePath(dst));
		}
	});
	
	auto doSort = [=, this](){
		
		progBar->setMinimum(0);
		progBar->setMaximum(0);
		progBar->setValue(0);
		progBar->setFormat("Sorting...");
		
		struct AlgaeLink {
			AlgaeImage * image = nullptr;
			AlgaeLink * left = nullptr;
			AlgaeLink * right = nullptr;
			
			bool WouldLoop(AlgaeLink const * other) {
				if (this == other)
					return true;
				if (right) {
					AlgaeLink * r2 = right;
					while (true) {
						if (r2 == other)
							return true;
						if (!r2)
							break;
						r2 = r2->right;
					}
				}
				if (left) {
					AlgaeLink * l2 = left;
					while (true) {
						if (l2 == other)
							return true;
						if (!l2)
							break;
						l2 = l2->left;
					}
				}
				return false;
			}
			
			AlgaeLink * CompleteLoop() {
				AlgaeLink * l = this;
				AlgaeLink * r = this;
				
				if (left) {
					while (true) {
						if (!l->left)
							break;
						l = l->left;
					}
				}
				
				if (right) {
					while (true) {
						if (!r->right)
							break;
						r = r->right;
					}
				}
				
				l->left = r;
				r->right = l;
				
				return l;
			}
		};
		
		QList<AlgaeLink> links;
		links.resize(m_images.size());
		
		for ( int i = 0; i < links.size(); i++ ) {
			links[i].image = &m_images[i];
		}
		
		for (auto & comp : m_comp) {
			AlgaeLink & x = links[comp.x];
			AlgaeLink & y = links[comp.y];
			
			if (x.left && x.right)
				continue;
			
			if (y.left && y.right)
				continue;
			
			if (x.WouldLoop(&y))
				continue;
			
			if (!x.right && !y.left) {
				x.right = &y;
				y.left = &x;
			} else if (!x.left && !y.right) {
				x.left = &y;
				y.right = &x;
			} else if (!x.left && !y.left) {
				AlgaeLink * y2 = &y;
				while (true) {
					std::swap(y2->left, y2->right);
					if (!y2->left) break;
					y2 = y2->left;
				}
				assert(!x.left && !y.right);
				x.left = &y;
				y.right = &x;
			} else if (!x.right && !y.right) {
				AlgaeLink * y2 = &y;
				while (true) {
					std::swap(y2->left, y2->right);
					if (!y2->right) break;
					y2 = y2->right;
				}
				assert(!x.right && !y.left);
				x.right = &y;
				y.left = &x;
			}
		}
		
		AlgaeLink * cur = links[0].CompleteLoop();
		for ( int i = 0; i < links.size(); i++ ) {
			m_images[i].sortValue = std::numeric_limits<decltype(m_images[i].sortValue)>::max();
		}
		for ( int i = 0; cur && i < links.size(); i++ ) {
			cur->image->sortValue = i;
			//qDebug() << QString("Link %0: %1").arg(i).arg(cur->image - m_images.data());
			cur = cur->right;
		}
		//qDebug() << "================";
		
		regenerateLinks();
		displayImages();
		
		progBar->setMinimum(0);
		progBar->setMaximum(1);
		progBar->setValue(1);
		progBar->setFormat("Done");
		
		renameBut->setDisabled(false);
	};
	
	connect(sortBut1, &QPushButton::clicked, [=, this](){
		
		struct {
			QMutex progMut;
			QAtomicInteger<int64_t> w_i = 0;
			std::chrono::steady_clock::time_point lastUp = std::chrono::steady_clock::now();
		} deltaWorkData;

		auto deltaWorkFunc = [=, this, &deltaWorkData](){
			while (true) {
				int64_t i = deltaWorkData.w_i++;
				if (i >= m_comp.size())
					break;
				
				AlgaePairComp & apc = m_comp[i];
				apc.value = AlgaeImage::ComparePix( m_images[apc.x], m_images[apc.y] );
				
				deltaWorkData.progMut.lock();
				auto now = std::chrono::steady_clock::now();
				if (std::chrono::duration_cast<std::chrono::milliseconds>(now - deltaWorkData.lastUp).count() > 50) {
					progBar->setValue(i);
					deltaWorkData.lastUp = now;
				}
				deltaWorkData.progMut.unlock();
			}
		};
		
		progBar->setMinimum(0);
		progBar->setFormat("Generating Deltas %p%");
		progBar->setValue(0);
		progBar->setMaximum(m_comp.size());
		
		std::vector<std::thread> threads;
		for (size_t i = 0; i < std::thread::hardware_concurrency() / 2; i++) threads.emplace_back(deltaWorkFunc);
		for (auto & t : threads) t.join();
		threads.clear();
		
		progBar->setMinimum(0);
		progBar->setMaximum(0);
		progBar->setValue(0);
		progBar->setFormat("Sorting...");
		
		std::sort(m_comp.begin(), m_comp.end());
		
		regenerateLinks();
		displayImages();
		
		doSort();
	});
	
	connect(sortBut2, &QPushButton::clicked, [=, this](){
		
		struct {
			QMutex progMut;
			QAtomicInteger<int64_t> w_i = 0;
			std::chrono::steady_clock::time_point lastUp = std::chrono::steady_clock::now();
		} deltaWorkData;

		auto deltaWorkFunc = [=, this, &deltaWorkData](){
			while (true) {
				int64_t i = deltaWorkData.w_i++;
				if (i >= m_comp.size())
					break;
				
				AlgaePairComp & apc = m_comp[i];
				apc.value = AlgaeImage::CompareHist( m_images[apc.x], m_images[apc.y] );
				
				deltaWorkData.progMut.lock();
				auto now = std::chrono::steady_clock::now();
				if (std::chrono::duration_cast<std::chrono::milliseconds>(now - deltaWorkData.lastUp).count() > 50) {
					progBar->setValue(i);
					deltaWorkData.lastUp = now;
				}
				deltaWorkData.progMut.unlock();
			}
		};
		
		progBar->setMinimum(0);
		progBar->setFormat("Generating Deltas %p%");
		progBar->setValue(0);
		progBar->setMaximum(m_comp.size());
		
		std::vector<std::thread> threads;
		for (size_t i = 0; i < std::thread::hardware_concurrency() / 2; i++) threads.emplace_back(deltaWorkFunc);
		for (auto & t : threads) t.join();
		threads.clear();
		
		progBar->setMinimum(0);
		progBar->setMaximum(0);
		progBar->setValue(0);
		progBar->setFormat("Sorting...");
		
		std::sort(m_comp.begin(), m_comp.end());
		
		regenerateLinks();
		displayImages();
		
		doSort();
	});
	
	connect(sortBut3, &QPushButton::clicked, [=, this](){
		
		struct {
			QMutex progMut;
			QAtomicInteger<int64_t> w_i = 0;
			std::chrono::steady_clock::time_point lastUp = std::chrono::steady_clock::now();
		} deltaWorkData;

		auto deltaWorkFunc = [=, this, &deltaWorkData](){
			while (true) {
				int64_t i = deltaWorkData.w_i++;
				if (i >= m_comp.size())
					break;
				
				AlgaePairComp & apc = m_comp[i];
				apc.value = 0;
				apc.value += 0.3 * AlgaeImage::ComparePix( m_images[apc.x], m_images[apc.y] );
				apc.value += 0.7 * AlgaeImage::CompareHist( m_images[apc.x], m_images[apc.y] );
				
				deltaWorkData.progMut.lock();
				auto now = std::chrono::steady_clock::now();
				if (std::chrono::duration_cast<std::chrono::milliseconds>(now - deltaWorkData.lastUp).count() > 50) {
					progBar->setValue(i);
					deltaWorkData.lastUp = now;
				}
				deltaWorkData.progMut.unlock();
			}
		};
		
		progBar->setMinimum(0);
		progBar->setFormat("Generating Deltas %p%");
		progBar->setValue(0);
		progBar->setMaximum(m_comp.size());
		
		std::vector<std::thread> threads;
		for (size_t i = 0; i < std::thread::hardware_concurrency() / 2; i++) threads.emplace_back(deltaWorkFunc);
		for (auto & t : threads) t.join();
		threads.clear();
		
		progBar->setMinimum(0);
		progBar->setMaximum(0);
		progBar->setValue(0);
		progBar->setFormat("Sorting...");
		
		std::sort(m_comp.begin(), m_comp.end());
		
		regenerateLinks();
		displayImages();
		
		doSort();
	});
	
	connect(initBut, &QPushButton::clicked, [=, this](){
		initBut->setDisabled(true);
		
		struct {
			QMutex progMut;
			QAtomicInt w_i = 0;
		} loadWorkData;
		
		auto loadWorkFunc = [=, this, &loadWorkData](){
			while (true) {
				int i = loadWorkData.w_i++;
				if (i >= m_images.size())
					break;
				AlgaeImage & ai = m_images[i];
				ai.Initialize();
				loadWorkData.progMut.lock();
				progBar->setValue(progBar->value() + 1);
				loadWorkData.progMut.unlock();
			}
		};
		
		uint loadThreadCount = std::min<uint>(6, std::thread::hardware_concurrency() / 2);
		progBar->setMinimum(0);
		progBar->setFormat("Loading %p%");
		progBar->setValue(0);
		progBar->setMaximum(m_images.size());
		
		std::vector<std::thread> threads;
		for (size_t i = 0; i < loadThreadCount; i++) threads.emplace_back(loadWorkFunc);
		for (auto & t : threads) t.join();
		threads.clear();
		
		m_images.removeIf([](AlgaeImage const & img){ return !img.valid; });
		qDebug() << m_images.size() << "images loaded and validated";
		
		int64_t compMax = ((std::pow(m_images.size(), 2ULL) + m_images.size()) / 2ULL) - m_images.size();
		m_comp.reserve(compMax);
		qDebug() << compMax << "unique delta pairs";
		
		for ( int64_t x = 0; x < m_images.size(); x++ ) for ( int64_t y = 0; y < x; y++ ) {
			if (x == y) continue;
			
			auto & ai = m_comp.emplace_back();
			ai.x = x;
			ai.y = y;
		}
		
		regenerateLinks();
		displayImages();
		
		progBar->setMinimum(0);
		progBar->setMaximum(1);
		progBar->setValue(1);
		progBar->setFormat("Done");
		
		sortBut1->setDisabled(false);
		sortBut2->setDisabled(false);
		sortBut3->setDisabled(false);
	});
}

void AlgaeImage::Initialize() {
	QImage img { file.canonicalFilePath() };
	if (img.isNull())
		return;
	this->thumb.convertFromImage(img.scaled(AlgaeConstants::THUMBNAIL_SIZE, AlgaeConstants::THUMBNAIL_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	
	QImage testI = img.scaled(AlgaeConstants::TEST_SIZE, AlgaeConstants::TEST_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).convertedTo(QImage::Format_RGB32);
	this->test.reserve(testI.width() * testI.height() * 3);
	for (int x = 0; x < AlgaeConstants::TEST_SIZE; x++) for (int y = 0; y < AlgaeConstants::TEST_SIZE; y++) {
		QRgb c = testI.pixel(x, y);
		test.push_back( qRed(c) );
		test.push_back( qGreen(c) );
		test.push_back( qBlue(c) );
	}
	
	cv::Mat cvm = cv::Mat(AlgaeConstants::TEST_SIZE, AlgaeConstants::TEST_SIZE, CV_8UC3, (void *)test.data());
	std::vector<cv::Mat> bgr_planes;
    cv::split( cvm, bgr_planes );
	
	int histSize = 256;
    float range[] = { 0, 256 }; //the upper boundary is exclusive
    const float* histRange[] = { range };
	
	calcHist(&bgr_planes[0], 1, 0, cv::Mat(), b_hist, 1, &histSize, histRange, true, false);
	calcHist(&bgr_planes[1], 1, 0, cv::Mat(), g_hist, 1, &histSize, histRange, true, false);
	calcHist(&bgr_planes[2], 1, 0, cv::Mat(), r_hist, 1, &histSize, histRange, true, false);
	
	normalize(b_hist, b_hist, 1.0, 0.0, cv::NORM_L1);
	normalize(g_hist, g_hist, 1.0, 0.0, cv::NORM_L1);
	normalize(r_hist, r_hist, 1.0, 0.0, cv::NORM_L1);
	
	valid = true;
}

double AlgaeImage::ComparePix(AlgaeImage const & A, AlgaeImage const & B) {
	
	double match = 0;
	uint_fast16_t res = AlgaeConstants::TEST_SIZE;
	
	for (uint_fast32_t i = 0; i < res * res; i++) {
		
		QRgb const & cA = A.test[i];
		QRgb const & cB = B.test[i];
		
		uint mR = abs(qRed(cA) - qRed(cB));
		uint mG = abs(qGreen(cA) - qGreen(cB));
		uint mB = abs(qBlue(cA) - qBlue(cB));
		double baseV = (mR + mG + mB) / 255.0;
		
		match += (3.0 - baseV) / 3.0;
	}
	
	return match / (res * res);
}


double AlgaeImage::CompareHist(AlgaeImage const & A, AlgaeImage const & B) {
	
	double rH = compareHist(A.r_hist, B.r_hist, cv::HISTCMP_BHATTACHARYYA);
	double gH = compareHist(A.g_hist, B.g_hist, cv::HISTCMP_BHATTACHARYYA);
	double bH = compareHist(A.b_hist, B.b_hist, cv::HISTCMP_BHATTACHARYYA);
	double sH = (rH + gH + bH) / 3;
	
	return 1 - sH;
}
