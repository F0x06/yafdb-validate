#include "ymlparser.h"
#include <QDebug>

YMLParser::YMLParser()
{
}


void YMLParser::writeItem(cv::FileStorage &fs, ObjectRect* obj)
{
    // Write class name name
    switch(obj->getType())
    {
    case ObjectType::Face:
        fs << "className" << "Face";
        break;
    case ObjectType::NumberPlate:
        fs << "className" << "NumberPlate";
        break;
    case ObjectType::ToBlur:
        fs << "className" << "ToBlur";
        break;
    case ObjectType::None:
        fs << "className" << "None";
        break;
    }

    // Write square area coodinates
    fs << "area" << "{";
    fs << "p1" << cv::Point2d(obj->proj_point_1().x(), obj->proj_point_1().y());
    fs << "p2" << cv::Point2d(obj->proj_point_2().x(), obj->proj_point_2().y());
    fs << "p3" << cv::Point2d(obj->proj_point_3().x(), obj->proj_point_3().y());
    fs << "p4" << cv::Point2d(obj->proj_point_4().x(), obj->proj_point_4().y());
    fs << "}";

    // Write params
    fs << "params" << "{";
    fs << "azimuth" << obj->proj_azimuth();
    fs << "elevation" << obj->proj_elevation();
    fs << "aperture" << obj->proj_aperture();
    fs << "width" << obj->proj_width();
    fs << "height" << obj->proj_height();
    fs << "}";

    qDebug() << "Write AZ: " << obj->proj_azimuth();
    qDebug() << "Write EL: " << obj->proj_elevation();
    qDebug() << "Write AP: " << obj->proj_aperture();
    qDebug() << "Write W: " << obj->proj_width();
    qDebug() << "Write H: " << obj->proj_height();

    // Write status tags
    fs << "autoStatus" << obj->getAutomaticStatus().toStdString();
    fs << "manualStatus" << obj->getManualStatus().toStdString();
}

ObjectRect* YMLParser::readItem(cv::FileNodeIterator iterator)
{
    // Initialize detected object
    ObjectRect* object = new ObjectRect;

    // Parse class name
    std::string className;
    (*iterator)["className"] >> className;

    if(className == "Face")
    {
        object->setType( ObjectType::Face );
    } else if(className == "NumberPlate") {
        object->setType( ObjectType::NumberPlate );
    } else if(className == "ToBlur") {
        object->setType( ObjectType::ToBlur );
    } else if(className == "None") {
        object->setType( ObjectType::None );
    }

    // Parse area points
    cv::FileNode areaNode = (*iterator)["area"];
    cv::Point2d pt_1;
    cv::Point2d pt_2;
    cv::Point2d pt_3;
    cv::Point2d pt_4;

    areaNode["p1"] >> pt_1;
    areaNode["p2"] >> pt_2;
    areaNode["p3"] >> pt_3;
    areaNode["p4"] >> pt_4;

    object->setPoints(QPointF(pt_1.x, pt_1.y),
                      QPointF(pt_2.x, pt_2.y),
                      QPointF(pt_3.x, pt_3.y),
                      QPointF(pt_4.x, pt_4.y));
    //object->setProjectionPoints();

    // Parse gnomonic params
    cv::FileNode paramsNode = (*iterator)["params"];
    float azimuth = 0.0;
    float elevation = 0.0;
    float aperture = 0.0;
    float width = 0.0;
    float height = 0.0;

    paramsNode["azimuth"] >> azimuth;
    paramsNode["elevation"] >> elevation;
    paramsNode["aperture"] >> aperture;
    paramsNode["width"] >> width;
    paramsNode["height"] >> height;


    qDebug() << "Read AZ: " << azimuth;
    qDebug() << "Read EL: " << elevation;
    qDebug() << "Read AP: " << aperture;
    qDebug() << "Read W: " << width;
    qDebug() << "Read H: " << height;

    object->setProjectionParametters(azimuth,
                                    elevation,
                                    aperture,
                                    width,
                                    height);

    object->setProjectionParametters(azimuth, elevation, aperture, width, height);
    object->setProjectionPoints();

    // Parse auto status
    std::string autoStatus;
    (*iterator)["autoStatus"] >> autoStatus;
    object->setAutomaticStatus( QString(autoStatus.c_str()) );
    object->setAutomaticStatus( (object->getAutomaticStatus().length() > 0 ? object->getAutomaticStatus() : "None") );

    // Parse manual status
    std::string manualStatus;
    (*iterator)["manualStatus"] >> manualStatus;
    object->setManualStatus( QString(manualStatus.c_str()) );
    object->setManualStatus( (object->getManualStatus().length() > 0 ? object->getManualStatus() : "None") );

    cv::FileNode childNode = (*iterator)["childrens"];
    for (cv::FileNodeIterator child = childNode.begin(); child != childNode.end(); ++child) {
        object->childrens.append( this->readItem( child ) );
    }

    // Return object
    return object;
}

void YMLParser::writeYML(QList<ObjectRect*> objects, QString path)
{
    // Open storage for writing
    cv::FileStorage fs(path.toStdString(), cv::FileStorage::WRITE);

    // Write source file path
    fs << "source_image" << objects.first()->getSourceImagePath().toStdString();

    // Write objects
    fs << "objects" << "[";

    // Iterate over objects
    foreach (ObjectRect* obj, objects) {

        // Open array element
        fs << "{";

        // Object writer method
        this->writeItem(fs, obj);

        // Write childrens if present
        if(obj->childrens.length() > 0)
        {
            fs << "childrens" << "[";
            foreach (ObjectRect* child, obj->childrens) {
                fs << "{";
                this->writeItem(fs, child);
                fs << "}";
            }
            fs << "]";
        }

        // Close array element
        fs << "}";
    }

    // Close array
    fs << "]";
}

QList<ObjectRect*> YMLParser::loadYML(QString path)
{
    // Init output list
    QList<ObjectRect*> out_list;

    // Read YML file
    cv::FileStorage fs(path.toStdString(), cv::FileStorage::READ);

    // Retrieve objects node
    cv::FileNode objectsNode = fs["objects"];

    // Iterate over objects
    for (cv::FileNodeIterator it = objectsNode.begin(); it != objectsNode.end(); ++it) {

        // Initialize detected object
        ObjectRect* object = this->readItem(it);

        std::string source_image;
        fs["source_image"] >> source_image;

        object->setSourceImagePath( QString(source_image.c_str()) );

        // Append to list
        out_list.append(object);
    }

    // Return results
    return out_list;
}

