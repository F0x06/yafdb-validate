#include "editview.h"
#include "ui_editview.h"

EditView::EditView(QWidget *parent, ObjectRect* rect) :
    QMainWindow(parent),
    ui(new Ui::EditView)
{
    ui->setupUi(this);

    this->ref_rect = rect;

    this->pano_parent = qobject_cast<PanoramaViewer *>(parent);

    QStandardItemModel* model =
            qobject_cast<QStandardItemModel*>(this->ui->typeList->model());
    QModelIndex firstIndex = model->index(0, this->ui->typeList->modelColumn(),
            this->ui->typeList->rootModelIndex());
    QStandardItem* firstItem = model->itemFromIndex(firstIndex);
    firstItem->setSelectable(false);

    // Connect signal for labels refresh
    connect(this, SIGNAL(refreshLabels()), parent, SLOT(refreshLabels_slot()));

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
    this->ui->mainLayout->addWidget(this->pano);

    // Configure panorama viewer
    this->pano->setup(
        this->size().width(), // Default width
        this->size().height(), // Default height
        pano_parent->scale_factor,   // Image scale factor
        pano_parent->zoom_min,  // Minimum zoom
        pano_parent->zoom_max, // Maximum zoom
        100.0, // Default zoom level
        pano_parent->threads_count // Number of threads
    );

    this->pano->setView( rect->proj_azimuth(), rect->proj_elevation() );
    this->pano->setZoom( rect->proj_aperture() / (LG_PI / 180.0) );

    this->pano->setMoveEnabled( false );
    this->pano->setZoomEnabled( false );
    this->pano->setCreateEnabled( false );
    this->pano->setEditEnabled( false );

    // Load input image
    this->pano->loadImage( pano_parent->src_image );

    qDebug() << this->pano->size();

    // Set-up labels
    this->ui->subClassLabel->setText("Sub classes: None");

    switch(this->ref_rect->getType())
    {
        case ObjectType::None:
            this->ui->clannNameLabel->setText("Class name: None");
            break;
        case ObjectType::Face:
            this->ui->clannNameLabel->setText("Class name: Face");
            break;
        case ObjectType::NumberPlate:
            this->ui->clannNameLabel->setText("Class name: NumberPlate");
            break;
        case ObjectType::ToBlur:
            this->ui->clannNameLabel->setText("Class name: ToBlur");
            break;
    }

    this->ui->widthLabel->setText("Width: " + QString::number( (int) this->ref_rect->getSize().width() ));
    this->ui->heightLabel->setText("Height: " + QString::number( (int) this->ref_rect->getSize().height() ));
    this->ui->preFiltersLabel->setText("Pre-filter status: " + this->ref_rect->getAutomaticStatus());

    this->rect_copy = this->ref_rect->copy();

    this->rect_copy->mapTo(this->pano->dest_image_map.width(),
                     this->pano->dest_image_map.height(),
                     this->rect_copy->proj_azimuth(),
                     this->rect_copy->proj_elevation(),
                     this->rect_copy->proj_aperture());

    this->pano->rect_list.append( this->rect_copy );
    this->pano->scene->addItem( this->rect_copy );

}

EditView::~EditView()
{
    delete ui;
}

void EditView::on_cancelButton_clicked()
{
    this->close();
}

void EditView::on_deleteButton_clicked()
{
    pano_parent->rect_list.removeOne( this->ref_rect );
    delete this->ref_rect;
    emit refreshLabels();
    this->close();
}

void EditView::on_confirmButton_clicked()
{

    foreach(ObjectRect* rect, this->pano_parent->rect_list)
    {
        if(rect->getId() == this->rect_copy->getId())
        {
            this->rect_copy->mapTo(rect->proj_width(),
                                   rect->proj_height(),
                                   rect->proj_azimuth(),
                                   rect->proj_elevation(),
                                   rect->proj_aperture()
                                   );

            rect->setProjectionPoints(this->rect_copy->getPoint1(),
                                      this->rect_copy->getPoint2(),
                                      this->rect_copy->getPoint3(),
                                      this->rect_copy->getPoint4());
        }
    }

    this->pano_parent->render();

    this->close();
}
