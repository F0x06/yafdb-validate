#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QDebug>
#include <QGraphicsProxyWidget>

#include "ymlparser.h"

MainWindow::MainWindow(QWidget *parent, QString sourceImagePath, QString detectorYMLPath, QString destinationYMLPath) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->initializeValidator(sourceImagePath, detectorYMLPath, destinationYMLPath);
}

void MainWindow::initializeValidator(QString sourceImagePath, QString detectorYMLPath, QString destinationYMLPath)
{
    ui->setupUi(this);

    this->options.sourceImagePath = sourceImagePath.length() > 0 ? sourceImagePath : "";
    this->options.detectorYMLPath = detectorYMLPath.length() > 0 ? detectorYMLPath : "";
    this->options.destinationYMLPath = destinationYMLPath.length() > 0 ? destinationYMLPath : "";

    // Determine good labels colors based on system theme
    QString good_color_string = "rgb(%1, %2, %3)";
    QColor  good_color_color = QApplication::palette().color(QPalette::Text);
    good_color_string = good_color_string.arg(good_color_color.red()).arg(good_color_color.green()).arg(good_color_color.blue());

    QString warn_color_string = "rgb(%1, %2, %3)";
    QColor  warn_color_color = QApplication::palette().color(QPalette::Highlight);
    warn_color_string = warn_color_string.arg(warn_color_color.red()).arg(warn_color_color.green()).arg(warn_color_color.blue());

    this->good_color = good_color_string;
    this->warn_color = warn_color_string;

    new QShortcut(QKeySequence("Esc"), this, SLOT(onESC()));

    // Remove margins
    this->setContentsMargins(-5, -5, -5, -5);
    //this->centralWidget()->layout()->setContentsMargins(50,50,50,50);

    // Center window on screen
    this->setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            this->size(),
            qApp->desktop()->availableGeometry()
        ));

    // Start window maximized
    this->showMaximized();

    // Create panorama viewer
    this->pano = new PanoramaViewer(this);

    // Add panorama viewer to current window
    this->ui->gridLayout->addWidget(this->pano);

    // Determine best multi-threading setup
    int threads_count = QThread::idealThreadCount();

    // Configure panorama viewer
    this->pano->setup(
        this->size().width(), // Default width
        this->size().height(), // Default height
        0.6,   // Image scale factor
        20.0,  // Minimum zoom
        100.0, // Maximum zoom
        100.0, // Default zoom level
        threads_count // Number of threads
    );

    //this->ui->horizontalSlider->setValue(this->pano->scale_factor * 100);

    bool sourceImageFile_exists = false;
    bool detectorYMLFile_exists = false;
    bool destinationYMLFile_exists = false;

    if( this->options.sourceImagePath.length() > 0 )
    {
        QFileInfo sourceImageFile( this->options.sourceImagePath );
        sourceImageFile_exists = ( sourceImageFile.exists() && sourceImageFile.isFile( ));
    }

    if( this->options.detectorYMLPath.length() > 0 )
    {
        QFileInfo detectorYMLFile( this->options.detectorYMLPath );
        detectorYMLFile_exists = ( detectorYMLFile.exists() && detectorYMLFile.isFile( ));
    }

    if( this->options.destinationYMLPath.length() > 0 )
    {
        QFileInfo destinationYMLFile( this->options.destinationYMLPath );
        destinationYMLFile_exists = ( destinationYMLFile.exists() && destinationYMLFile.isFile( ));
    }

    if( !sourceImageFile_exists )
    {
        std::cout << "[ERROR] Invalid source image path: " << this->options.sourceImagePath.toStdString() << std::endl;
        exit( 0 );
    }

    if( this->options.detectorYMLPath.length() > 0 && !detectorYMLFile_exists )
    {
        std::cout << "[ERROR] Invalid detector YML path: " << this->options.detectorYMLPath.toStdString() << std::endl;
    }

    // Load input image
    this->pano->loadImage( this->options.sourceImagePath );

    YMLParser parser;

    if( this->options.detectorYMLPath.length() > 0 )
    {
        if( detectorYMLFile_exists )
        {
            if( destinationYMLFile_exists )
            {
                QList<ObjectRect*> loaded_rects = parser.loadYML( this->options.destinationYMLPath, YMLType::Validator );

                foreach(ObjectRect* rect, loaded_rects)
                {

                    rect->mapTo(this->pano->dest_image_map.width(),
                                this->pano->dest_image_map.height(),
                                this->pano->position.azimuth,
                                this->pano->position.elevation,
                                this->pano->position.aperture);

                    this->pano->rect_list.append( rect );
                    this->pano->scene->addItem( rect );

                    if( !this->pano->isObjectVisible( rect ) )
                        rect->setVisible( false );
                }
            } else {
                QList<ObjectRect*> loaded_rects = parser.loadYML( this->options.detectorYMLPath, YMLType::Detector );

                foreach(ObjectRect* rect, loaded_rects)
                {

                    rect->mapFromSpherical(this->pano->image_info.width,
                                           this->pano->image_info.height,
                                           this->pano->dest_image_map.width(),
                                           this->pano->dest_image_map.height(),
                                           this->pano->position.azimuth,
                                           this->pano->position.elevation,
                                           this->pano->position.aperture,
                                           this->pano->zoom_min * ( LG_PI / 180.0 ),
                                           this->pano->zoom_max * ( LG_PI / 180.0 ));

                    this->pano->rect_list.append( rect );
                    this->pano->scene->addItem( rect );

                    if( !this->pano->isObjectVisible( rect ) )
                        rect->setVisible( false );
                }
            }
        }
    } else {
        if( this->options.destinationYMLPath.length() > 0 )
        {
            if( destinationYMLFile_exists )
            {
                QList<ObjectRect*> loaded_rects = parser.loadYML( this->options.destinationYMLPath, YMLType::Validator );

                foreach(ObjectRect* rect, loaded_rects)
                {

                    rect->mapTo(this->pano->dest_image_map.width(),
                                this->pano->dest_image_map.height(),
                                this->pano->position.azimuth,
                                this->pano->position.elevation,
                                this->pano->position.aperture);

                    this->pano->rect_list.append( rect );
                    this->pano->scene->addItem( rect );

                    if( !this->pano->isObjectVisible( rect ) )
                        rect->setVisible( false );
                }
            }
        } else {
            exit( 0 );
        }
    }

    this->output_yml = this->options.destinationYMLPath;

    // Initialize labels
    emit refreshLabels();
}

void MainWindow::refreshLabels()
{
    int untyped = 0;

    int facecount = 0;
    int facesvalidated = 0;

    int numberplatescount = 0;
    int numberplatesvalidated = 0;

    int preinvalidatedcount = 0;
    int preinvalidatedvalidated = 0;

    int toblurcount = 0;

    foreach(ObjectRect* rect, this->pano->rect_list)
    {
        switch(rect->getType())
        {
        case ObjectType::None:
            untyped++;
            break;
        case ObjectType::Face:

            if(rect->getAutomaticStatus() == "Valid" || rect->getAutomaticStatus() == "None")
                facecount++;

            if( (rect->getAutomaticStatus() == "Valid" || rect->getAutomaticStatus() == "None")
                    && rect->getManualStatus() != "None")
            {
                facesvalidated++;
            }

            if(rect->getAutomaticStatus() != "None" && rect->getAutomaticStatus() != "Valid")
            {
                preinvalidatedcount++;
            }
            if(rect->getAutomaticStatus() != "None" && rect->getAutomaticStatus() != "Valid" && rect->getManualStatus() != "None")
            {
                preinvalidatedvalidated++;
            }

            break;
        case ObjectType::NumberPlate:

            numberplatescount++;

            if(rect->getManualStatus() != "None")
            {
                numberplatesvalidated++;
            }

            break;
        case ObjectType::ToBlur:

            toblurcount++;
            break;
        }
    }

    if(untyped > 0)
    {
        this->ui->untypedLabel->setStyleSheet("QLabel {color: " + this->warn_color + "; }");
        this->ui->untypedButton->setEnabled(true);
    } else {
        this->ui->untypedLabel->setStyleSheet("QLabel {color: " + this->good_color + "; }");
        this->ui->untypedButton->setEnabled(false);
    }

    if(facecount <= 0)
    {
        this->ui->facesButton->setEnabled(false);
    } else {
        this->ui->facesButton->setEnabled(true);
    }

    if(numberplatescount <= 0)
    {
        this->ui->platesButton->setEnabled(false);
    } else {
        this->ui->platesButton->setEnabled(true);
    }

    if(preinvalidatedcount <= 0)
    {
        this->ui->preInvalidatedButton->setEnabled(false);
    } else {
        this->ui->preInvalidatedButton->setEnabled(true);
    }

    if(toblurcount <= 0)
    {
        this->ui->toBlurButton->setEnabled(false);
    } else {
        this->ui->toBlurButton->setEnabled(true);
    }

    if(facesvalidated != facecount)
    {
        this->ui->facesLabel->setStyleSheet("QLabel {color: " + this->warn_color + "; }");
    } else {
        this->ui->facesLabel->setStyleSheet("QLabel {color: " + this->good_color + "; }");
    }

    if(numberplatesvalidated != numberplatescount)
    {
        this->ui->platesLabel->setStyleSheet("QLabel {color: " + this->warn_color + "; }");
    } else {
        this->ui->platesLabel->setStyleSheet("QLabel {color: " + this->good_color + "; }");
    }

    if(preinvalidatedvalidated != preinvalidatedcount)
    {
        this->ui->preInvalidatedLabel->setStyleSheet("QLabel {color: " + this->warn_color + "; }");
    } else {
        this->ui->preInvalidatedLabel->setStyleSheet("QLabel {color: " + this->good_color + "; }");
    }

    this->ui->untypedLabel->setText("Untyped items: " + QString::number(untyped));
    this->ui->facesLabel->setText("Faces: " + QString::number(facesvalidated) + "/" + QString::number(facecount));
    this->ui->platesLabel->setText("Number plates: " + QString::number(numberplatesvalidated) + "/" + QString::number(numberplatescount));
    this->ui->preInvalidatedLabel->setText("Pre-invalidated: " + QString::number(preinvalidatedvalidated) + "/" + QString::number(preinvalidatedcount));
    this->ui->toBlurLabel->setText("To blur: " + QString::number(toblurcount));

}

void MainWindow::updateScaleSlider(int value)
{
    this->ui->horizontalSlider->setValue( value );
}

MainWindow::~MainWindow()
{
    //delete this->pano;
    delete ui;
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "",
                                                                tr("Do you want to save your work\n(You can resume later)"),
                                                                QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn == QMessageBox::Cancel) {
        event->ignore();
    } else if( resBtn == QMessageBox::Yes ) {
        YMLParser parser;
        parser.writeYML( this->pano->rect_list, this->output_yml );

        event->accept();
    } else if(resBtn == QMessageBox::No) {
        event->accept();
    }
}

void MainWindow::onESC()
{
    this->close();
}

void MainWindow::on_untypedButton_clicked()
{
    BatchView* w = new BatchView(this, this->pano, BatchMode::Manual, BatchViewMode::OnlyUntyped);
    w->setAttribute( Qt::WA_DeleteOnClose );
    w->show();
}

void MainWindow::on_facesButton_clicked()
{
    BatchView* w = new BatchView(this, this->pano, BatchMode::Auto, BatchViewMode::OnlyFaces);
    w->setAttribute( Qt::WA_DeleteOnClose );
    w->show();
}

void MainWindow::on_platesButton_clicked()
{
    BatchView* w = new BatchView(this, this->pano, BatchMode::Auto, BatchViewMode::OnlyNumberPlates);
    w->setAttribute( Qt::WA_DeleteOnClose );
    w->show();
}

void MainWindow::on_preInvalidatedButton_clicked()
{
    BatchView* w = new BatchView(this, this->pano, BatchMode::Auto, BatchViewMode::OnlyPreInvalidated);
    w->setAttribute( Qt::WA_DeleteOnClose );
    w->show();
}

void MainWindow::on_toBlurButton_clicked()
{
    BatchView* w = new BatchView(this, this->pano, BatchMode::ToBlur, BatchViewMode::OnlyToBlur);
    w->setAttribute( Qt::WA_DeleteOnClose );
    w->show();
}

void MainWindow::on_horizontalSlider_sliderMoved(int position)
{
    this->pano->backupPosition();
    this->pano->scale_factor = (position / 10.0);
    this->pano->render();
}
