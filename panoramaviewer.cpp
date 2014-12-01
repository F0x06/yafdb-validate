#include "panoramaviewer.h"
#include <QDebug>
#include <QApplication>
#include <QGraphicsProxyWidget>
#include <QToolTip>

#define CHANNELS_COUNT 4

PanoramaViewer::PanoramaViewer(QWidget *parent) :
    QGraphicsView(parent)
{

    //this->centralWidget()->layout()->setContentsMargins(50,50,50,50);

    // Remove side bars on the panorama viewer
    this->verticalScrollBar()->blockSignals(true);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->horizontalScrollBar()->blockSignals(true);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Initialize position container
    this->position.start_x = 0;
    this->position.start_y = 0;
    this->position.x = 0;
    this->position.y = 0;
    this->position.start_azimuth = 0.0;
    this->position.start_elevation = 0.0;
    this->position.azimuth = 0.0;
    this->position.elevation = 0.0;

    // Initialize default mode
    this->mode = Mode::None;

    // Configure default settings
    this->scale_factor = 1.0;
    this->zoom_min = 20.0;
    this->zoom_max = 120.0;
    this->current_zoom = 100.0;
    this->current_zoom_rad = ( this->current_zoom * ( LG_PI / 180.0 ) );
    this->threads_count = 1;
    this->pixmap_initialized = false;
    this->create_position.rect = NULL;
    this->selected_rect = NULL;

    // Create default scene
    this->scene = new QGraphicsScene();
    this->setScene(this->scene);

    this->def_rect_list.append(
                    new DetectedObject(
                                           "face",
                                           "valid",
                                           "",
                                           true,
                                           true,
                                           QPointF(1.3208951834245974e+00, 9.8656289914658829e-02),
                                           QPointF(1.3693934418425056e+00, 1.5755115975767880e-01),
                                           0.0,
                                           0.0,
                                           0.0
                                       )
                );

    this->def_rect_list.append(
                    new DetectedObject(
                                           "face",
                                           "valid",
                                           "",
                                           true,
                                           true,
                                           QPointF(1.0709177891907444e+00, 1.0662604278241428e-01),
                                           QPointF(1.1206981363900821e+00, 1.6912070899793463e-01),
                                           0.0,
                                           0.0,
                                           0.0
                                       )
                );

    this->def_rect_list.append(
                    new DetectedObject(
                                           "face",
                                           "valid",
                                           "",
                                           true,
                                           true,
                                           QPointF(2.4779305235198335e+00, 1.0673115405514666e-01),
                                           QPointF(2.5301383381627898e+00, 1.6929616941823034e-01),
                                           0.0,
                                           0.0,
                                           0.0
                                       )
                );

    this->def_rect_list.append(
                    new DetectedObject(
                                           "face",
                                           "valid",
                                           "",
                                           true,
                                           true,
                                           QPointF(4.4763739785197654e+00, 7.3852157575024627e-02),
                                           QPointF(4.5128031942315427e+00, 1.1540239830939350e-01),
                                           0.0,
                                           0.0,
                                           0.0
                                       )
                );


}

inline float clamp(float x, float a, float b)
{
    return x < a ? a : (x > b ? b : x);
}

float clampRad(float x, float a, float b)
{
    if(x > b)
    {
        float delta = (x - b);
        return (a + delta);
    } else if (x < a) {
        float delta = (a - x);
        return (b - delta);
    } else {
        return x;
    }
}

void PanoramaViewer::setup(float scale_factor, float zoom_min, float zoom_max, float zoom_def, int threads)
{
    this->scale_factor = scale_factor;
    this->zoom_min = zoom_min;
    this->zoom_max = zoom_max;
    this->current_zoom = zoom_def;
    this->current_zoom_rad = (zoom_def * (LG_PI / 180.0) );
    this->threads_count = threads;
}

void PanoramaViewer::loadImage(QString path)
{

    this->src_image.load(path);
    this->src_image_map = QPixmap::fromImage(this->src_image);

    this->render();

}

void PanoramaViewer::updateScene(float azimuth, float elevation, float zoom)
{

    int WIDTH = this->width();
    int HEIGHT = this->height();

    int DEST_WIDTH = WIDTH* scale_factor;
    int DEST_HEIGHT = HEIGHT * scale_factor;

    float AZIMUTH = azimuth * ( LG_PI / 180.0 );
    float ELEVATION = elevation * ( LG_PI / 180.0 );

    this->dest_image = QImage (DEST_WIDTH, DEST_HEIGHT, QImage::Format_RGB32);

    lg_etg_apperturep(

        ( inter_C8_t * ) this->src_image.bits(),
        this->src_image.width(),
        this->src_image.height(),
        CHANNELS_COUNT,
        ( inter_C8_t * ) dest_image.bits(),
        DEST_WIDTH,
        DEST_HEIGHT,
        CHANNELS_COUNT,
        AZIMUTH,
        ELEVATION,
        0.0,
        zoom,
        li_bilinearf,
        this->threads_count

    );

    this->dest_image_map = QPixmap::fromImage(this->dest_image);

    if(this->pixmap_initialized)
    {
        this->scene->removeItem(this->last_pixmap);
        delete this->last_pixmap;
    } else {
        this->pixmap_initialized = true;
    }

    this->last_pixmap = this->scene->addPixmap(this->dest_image_map);
    this->last_pixmap->setZValue(-1);

    this->scene->setSceneRect(this->dest_image_map.rect());
    this->fitInView(this->dest_image_map.rect());
}

void PanoramaViewer::render()
{
    this->updateScene(
        this->position.azimuth,
        this->position.elevation,
        this->current_zoom_rad
    );

    normalization_struct norm_params;
    norm_params.pano_width = this->scene->width();
    norm_params.pano_height = this->scene->height();
    norm_params.scale_factor = this->scale_factor;

    foreach(ObjectRect* rect, this->rect_list)
    {
        rect->update(norm_params);
    }
}

// Function to update view of current scene
void PanoramaViewer::setView(float azimuth, float elevation)
{
    // Update view
    this->position.azimuth = azimuth;
    this->position.elevation = elevation;

    // Render scene
    this->render();
}

// Function to update zoom of current scene
void PanoramaViewer::setZoom(float zoom_level)
{

    // Clamp input zoom value
    float zoom_clamped = clamp(zoom_level, this->zoom_min, this->zoom_max);

    // Convert zoom value to radians
    this->current_zoom_rad = ( zoom_clamped * ( LG_PI / 180.0 ) );

    // Render scene
    this->render();
}

// Mouse wheel handler
void PanoramaViewer::wheelEvent(QWheelEvent* event)
{
    // Determine delta
    int delta = (event->delta() / 120);

    // Update current zoom level
    this->current_zoom -= (delta * 1.5);

    // Apply zoom level
    this->setZoom(this->current_zoom);
}

// Mouse buttons click handler
void PanoramaViewer::mousePressEvent(QMouseEvent* event)
{

    // Store mouse coords
    int MouseX = event->x();
    int MouseY = event->y();

    normalization_struct norm_params;
    norm_params.pano_width = this->scene->width();
    norm_params.pano_height = this->scene->height();
    norm_params.scale_factor = this->scale_factor;


    /* Denormalize detected object dimensions */
    float p1x_d  = ((this->def_rect_list.at(0)->point_1.x() / (M_PI * 2.0)) * this->scene->width());
    float p1y_d  = ((this->def_rect_list.at(0)->point_1.y() / M_PI) * this->scene->width());

    qDebug() << p1x_d;
    qDebug() << p1y_d;

    // Store normalized mouse coords
    QPointF mouse_norm = util::normalize(QPointF(event->pos()), norm_params);

    // Check presence of left click
    if(event->buttons() & Qt::LeftButton)
    {
        // Enable mouse tracking
        this->setMouseTracking(true);

        // Switch in mooving mode
        this->mode = Mode::Move;

        // Store base positions (Used to determine offset to move in panorama later)
        this->position.start_x = MouseX;
        this->position.start_y = MouseY;

         // Store base directions (Used to determine offset to move in panorama later)
        this->position.start_azimuth = this->position.azimuth;
        this->position.start_elevation = this->position.elevation;

    }
    // Check presence of right click
    else if (event->buttons() & Qt::RightButton)
    {
        // Get clicked object
        qDebug() << "1";

        QGraphicsProxyWidget* widget_proxy = qgraphicsitem_cast<QGraphicsProxyWidget*>(this->itemAt(MouseX, MouseY));

        // Verify that clicked object is a widget and is not null
        if (widget_proxy != NULL)
        {

           // Convert object to ObjectRect
            this->selected_rect = qobject_cast<ObjectRect*>(widget_proxy->widget());

            qDebug() << this->selected_rect->point_1;
            qDebug() << this->selected_rect->point_2;
            qDebug() << this->selected_rect->point_3;
            qDebug() << this->selected_rect->point_4;

        }

        if(this->selected_rect)
        {

            qDebug() << "Mode::MoveCreate";
            // Store base positions (Used to determine offset to move in panorama later)
            this->position.start_x = MouseX;
            this->position.start_y = MouseY;

            // Determine mouse click pointer offset


            this->position.offset = QPointF(
                            mouse_norm.x() - this->selected_rect->point_1.x(),
                            mouse_norm.y() - this->selected_rect->point_1.y()
                        );

            // Switch in creation mode
            this->mode = Mode::MoveCreate;

            // Enable mouse tracking
            this->setMouseTracking(true);

        } else {

            qDebug() << "Mode::Create";

            // Switch in creation mode
            this->mode = Mode::Create;

            // Enable mouse tracking
            this->setMouseTracking(true);

            // Store base positions (Used to determine size and position of object later)
            this->create_position.start_x = MouseX;
            this->create_position.start_y = MouseY;

        }
    }
}

// Mouse buttons release handler
void PanoramaViewer::mouseReleaseEvent(QMouseEvent *)
{
    // Reset mode
    this->mode = Mode::None;

    // Reset temporary object
    this->create_position.rect = NULL;
    this->selected_rect = NULL;

    // Disable mouse tracking
    this->setMouseTracking(false);
}

// Mouse displacement handler
void PanoramaViewer::mouseMoveEvent(QMouseEvent* event)
{

    // Store mouse coords
    int MouseX = event->x();
    int MouseY = event->y();

    normalization_struct norm_params;
    norm_params.pano_width = this->scene->width();
    norm_params.pano_height = this->scene->height();
    norm_params.scale_factor = this->scale_factor;

    // Store normalized mouse coords
    QPointF mouse_norm = util::normalize(QPointF(event->pos()), norm_params);

    // Moving mouse section (move in panorama)
    if(this->mode == Mode::None)
    {

    }
    else if(this->mode == Mode::Move)
    {

        // Determine the displacement delta
        int delta_x = (MouseX - this->position.start_x);
        int delta_y = (MouseY - this->position.start_y);

        // Apply delta to azimuth and elevation
        float azimuth   = (this->position.start_azimuth   - ( (delta_x * this->current_zoom_rad) * 0.1 ) );
        float elevation = (this->position.start_elevation + ( (delta_y * this->current_zoom_rad) * 0.1 ) );

        // Clamp azimuth and elevation
        this->position.azimuth   = clampRad(azimuth, -360.0, 360.0);
        this->position.elevation = clamp(elevation, -90.0, 90.0);

        // Render scene
        this->render();

    }
    // Section creation section (create a new object)
    else if (this->mode == Mode::Create)
    {

        // Check if temporary object is created
        if(this->create_position.rect == NULL)
        {

            // Create selection object
            this->create_position.rect = new ObjectRect(0, RectType::Manual);
            this->create_position.rect->setValid(true);

            // Save projection parameters to it
            this->create_position.rect->projection_parameters.aperture  = this->current_zoom_rad;
            this->create_position.rect->projection_parameters.azimuth   = this->position.azimuth;
            this->create_position.rect->projection_parameters.elevation = this->position.elevation;
            this->create_position.rect->projection_parameters.scale_factor = this->scale_factor;

            // Add selection object to list
            this->rect_list.append(this->create_position.rect);

            QGraphicsProxyWidget *proxyWidget = new QGraphicsProxyWidget;
            proxyWidget->setWidget(this->create_position.rect);

            // Add selection object to scene
            this->scene->addItem(proxyWidget);

            // Move selection object to mouse coords
            this->create_position.rect->setPos(
                            mouse_norm,
                            mouse_norm,
                            norm_params
                        );

        } else  {

            this->create_position.rect->setPos(
                            this->create_position.rect->point_1,
                            mouse_norm,
                            norm_params,
                            RectMoveType::Only_Point2
                        );

            QToolTip::showText(event->globalPos(),
                               QString::number( this->create_position.rect->width() ) + "x" +
                               QString::number( this->create_position.rect->height() ),
                               this, rect() );


        }
    } else if(this->mode == Mode::MoveCreate)
    {

        this->selected_rect->moveRect(mouse_norm, norm_params, this->position.offset);

    }
}

// Viewer resize handler
void PanoramaViewer::resizeEvent(QResizeEvent *)
{

    // Store actual dimensions
    int WIDTH = this->width();
    int HEIGHT = this->height();

    // Check if dimensions has changed
    if(WIDTH != this->previous_width && HEIGHT != this->previous_height)
    {

        // Store new dimensions
        this->previous_width = WIDTH;
        this->previous_height = HEIGHT;

        // Render scene
        this->render();
    }
}

// Function to crop a selection object
QImage PanoramaViewer::cropObject(ObjectRect* rect)
{

    // Build normalisation parameters structure
    normalization_struct norm_params;
    norm_params.pano_width = this->scene->width();
    norm_params.pano_height = this->scene->height();
    norm_params.scale_factor = this->scale_factor;

    // Denormalize rect bounds
    QPointF point1_denorm = util::denormalize(rect->point_1, norm_params);
    QPointF point2_denorm = util::denormalize(rect->point_2, norm_params);

    // Convert points to a QRect
    QRect rect_sel(
                    QPoint(point1_denorm.x(), point1_denorm.y()),
                    QPoint(point2_denorm.x(), point2_denorm.y())
                );

    // Crop and return image
    return this->dest_image.copy(rect_sel);

}
