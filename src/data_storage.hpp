/***
 * Stores all important Data over the runtime and handle the database.
 */

#ifndef DATASTORAGE_HPP_
#define DATASTORAGE_HPP_

#include <math.h>
#include <geos/index/strtree/STRtree.h>
#include <geos/index/ItemVisitor.h>
#include <geos/geom/prep/PreparedPolygon.h>
#include <gdal/ogrsf_frmts.h> 
#include <gdal/ogr_api.h>
#include <string>

using namespace std;

class DataStorage {

    OGRSpatialReference sparef_webmercator;
    OGRCoordinateTransformation *srs_tranformation;
    OGRDataSource *data_source;
    OGRLayer *layer_ways;
    OGRLayer *layer_vehicle;
    OGRLayer *layer_nodes;
    OGRLayer *layer_sidewalks;
    OGRLayer *layer_intersects;
    geom::OGRFactory<> ogr_factory;
    geom::GEOSFactory<> geos_factory;
    string output_database;
    string output_filename;
    //const char *SRS = "WGS84";
    location_handler_type &location_handler;
    GeomOperate go;

    void create_table(OGRLayer *&layer, const char *name,
            OGRwkbGeometryType geometry) {

        const char* options[] = {"OVERWRITE=YES", nullptr};
        layer = data_source->CreateLayer(name, &sparef_wgs84, geometry,
                const_cast<char**>(options));
        if (!layer) {
            cerr << "Layer " << name << " creation failed." << endl;
            exit(1);
        }
    }

    void create_field(OGRLayer *&layer, const char *name, OGRFieldType type,
            int width) {

        OGRFieldDefn ogr_field(name, type);
        ogr_field.SetWidth(width);
        if (layer->CreateField(&ogr_field) != OGRERR_NONE) {
            cerr << "Creating " << name << " field failed." << endl;
            exit(1);
        }
    }

    void init_db() {
        OGRRegisterAll();

        OGRSFDriver* driver;
        driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(
            "postgresql");
            //"SQLite");
            

        if (!driver) {
            cerr << "postgresql" << " driver not available." << endl;
            //cerr << "SQLite" << " driver not available." << endl;
            exit(1);
        }

        CPLSetConfigOption("OGR_TRUNCATE", "YES");
        //CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", "FALSE");
        string connection_string = "PG:dbname=" + output_filename;
        data_source = driver->CreateDataSource(connection_string.c_str());
        if (!data_source) {
            cerr << "Creation of output file failed." << endl;
            exit(1);
        }

        sparef_wgs84.SetWellKnownGeogCS("WGS84");
        //sparef_webmercator.importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext  +no_defs");
        //srs_transformation = OGRCreateCoordinateTransformation(sparef_wgs84,
        //    sparef_webmercator);
        //if (!srs_transformation) {
            //cerr << "Transformation object creation failed." << endl;
            //exit(1);
        //}

        create_table(layer_ways, "ways", wkbLineString);
        create_field(layer_ways, "gid", OFTInteger, 10); 
        create_field(layer_ways, "class_id", OFTInteger, 10);
        create_field(layer_ways, "length", OFTReal, 10);
        create_field(layer_ways, "name", OFTString, 40);
        create_field(layer_ways, "osm_id", OFTString, 14); 

        create_table(layer_vehicle, "vehicle", wkbLineString);
        create_field(layer_vehicle, "gid", OFTInteger, 10); 
        create_field(layer_vehicle, "class_id", OFTInteger, 10);
        create_field(layer_vehicle, "sidewalk", OFTString, 1);
        create_field(layer_vehicle, "type", OFTString, 20);
        create_field(layer_vehicle, "lanes", OFTInteger, 10);
        create_field(layer_vehicle, "length", OFTReal, 10);
        create_field(layer_vehicle, "name", OFTString, 40);
        create_field(layer_vehicle, "osm_id", OFTString, 14); 

        create_table(layer_nodes, "nodes", wkbPoint);
        create_field(layer_nodes, "osm_id", OFTString, 14);
        create_field(layer_nodes, "orientations", OFTString, 7);
        create_field(layer_nodes, "angle", OFTReal, 10);
        //test what 4th parameter does

        create_table(layer_intersects, "intersects", wkbPoint);
        create_field(layer_intersects, "roads", OFTString, 50);

        create_table(layer_sidewalks, "sidewalks", wkbMultiLineString);
    }

    void order_clockwise(object_id_type node_id) {
        Location node_location; 
        Location last_location; 
        int vector_size = node_map[node_id].size();
        node_location = location_handler.get_node_location(node_id);
        last_location = location_handler.get_node_location(
                node_map[node_id][vector_size - 1].first);

        double angle_last = go.orientation(node_location, last_location);
        for (int i = vector_size - 1; i > 0; --i) {
            object_id_type test_id = node_map[node_id][i - 1].first;
            Location test_location;
            test_location = location_handler.get_node_location(test_id);
            double angle_test = go.orientation(node_location, test_location);
            if (angle_last < angle_test) {
                swap(node_map[node_id][i], node_map[node_id][i-1]);
            } else {
                break;
            }
        }
    }
        
//    const string get_timestamp(Timestamp timestamp) {
//        string time_str = timestamp.to_iso();
//        time_str.replace(10, 1, " ");
//        time_str.replace(19, 1, "");
//        return time_str;
//    }
//
//    string width2string(float &width) {
//        int rounded_width = static_cast<int> (round(width * 10));
//        string width_str = to_string(rounded_width);
//        if (width_str.length() == 1) {
//            width_str.insert(width_str.begin(), '0');
//        }
//        width_str.insert(width_str.end() - 1, '.');
//        return width_str;
//    }
//
//    void destroy_polygons() {
//        for (auto polygon : prepared_polygon_set) {
//            delete polygon;
//        }
//        for (auto multipolygon : multipolygon_set) {
//            delete multipolygon;
//        }
//    }
//
public:
    /***
     * node_map: Contains all first_nodes and last_nodes of found waterways with
     * the names and categories of the connected ways.
     * error_map: Contains ids of the potential error nodes (or mouths) to be
     * checked in pass 3.
     * error_tree: The potential error nodes remaining after pass 3 are stored
     * in here for a geometrical analysis in pass 5.
     * polygon_tree: contains prepared polygons of all water polygons except of
     * riverbanks found in pass 4. 
     */

    OGRSpatialReference sparef_wgs84;
    google::sparse_hash_set<VehicleRoad*> vehicle_road_set;
    google::sparse_hash_set<PedestrianRoad*> pedestrian_road_set;
    google::sparse_hash_map<object_id_type,
	    vector<pair<object_id_type, VehicleRoad*>>> node_map;
    google::sparse_hash_set<string> finished_connections;
    vector<geos::geom::Geometry*> pedestrian_geometries;
    vector<geos::geom::Geometry*> vehicle_geometries;
    vector<geos::geom::Geometry*> sidewalk_geometries;
    geos::geom::Geometry *geos_pedestrian_net;
    geos::geom::Geometry *geos_vehicle_net;
    geos::geom::Geometry *geos_sidewalk_net;
    //geos::geom::GeometryFactory geos_factory;

    explicit DataStorage(string outfile,
            location_handler_type &location_handler) :
            output_filename(outfile),
            location_handler(location_handler) {

        init_db(); 
	node_map.set_deleted_key(-1);
	finished_connections.set_deleted_key("");
        vehicle_road_set.set_deleted_key(nullptr);
        pedestrian_road_set.set_deleted_key(nullptr);
        //pedestrian_geometries = new vector<geos::geom::Geometry*>();
        //sidewalk_geometries = new vector<geos::geom::Geometry*>();
        //vehicle_geometries = new vector<geos::geom::Geometry*>();
    }

    ~DataStorage() {
        layer_ways->CommitTransaction();
        layer_nodes->CommitTransaction();
        layer_vehicle->CommitTransaction();
        layer_sidewalks->CommitTransaction();
        layer_intersects->CommitTransaction();

        OGRDataSource::DestroyDataSource(data_source);
        OGRCleanupAll();
    }


    /*PedestrianRoad *store_pedestrian_road(string name, geos::geom::Geometry *geometry,
        string type, double length, string osm_id) {

        PedestrianRoad *pedestrian_road = new PedestrianRoad(name, geometry,
                type, length, osm_id);
        pedestrian_road_set.insert(pedestrian_road);
	return pedestrian_road;
    }

    VehicleRoad *store_vehicle_road(string name, geos::geom::Geometry *geometry,
        char sidewalk, string type, int lanes, double length,
        string osm_id) {

        VehicleRoad *vehicle_road = new VehicleRoad(name, geometry, sidewalk,
                type, lanes, length, osm_id);
        vehicle_road_set.insert(vehicle_road);
        
	return vehicle_road;
    }*/

    void insert_ways() {
        int gid = 0;
        for (PedestrianRoad *road : pedestrian_road_set) {
            gid++;
            OGRFeature *feature;
            feature = OGRFeature::CreateFeature(layer_ways->GetLayerDefn());

            if (feature->SetGeometry(road->get_ogr_geom()) != OGRERR_NONE) {
                cerr << "Failed to create geometry feature for way: ";
                cerr << road->osm_id << endl;
            }

            feature->SetField("gid", gid);
            feature->SetField("class_id", 0);
            feature->SetField("length", road->length);
            feature->SetField("name", road->name.c_str());
            feature->SetField("osm_id", road->osm_id.c_str());

            if (layer_ways->CreateFeature(feature) != OGRERR_NONE) {
                cerr << "Failed to create ways feature." << endl;
            }
            OGRFeature::DestroyFeature(feature);
        }
    }

    void insert_vehicle() {
        int gid = 0;
        for (VehicleRoad *road : vehicle_road_set) {
            gid++;
            OGRFeature *feature;
            feature = OGRFeature::CreateFeature(layer_vehicle->GetLayerDefn());

            if (feature->SetGeometry(road->get_ogr_geom()) != OGRERR_NONE) {
                cerr << "Failed to create geometry feature for way: ";
                cerr << road->osm_id << endl;
            }

            feature->SetField("gid", gid);
            feature->SetField("class_id", 0);
            feature->SetField("sidewalk", road->sidewalk);
            feature->SetField("type", road->type.c_str());
            feature->SetField("lanes", road->lanes);
            feature->SetField("length", road->length);
            feature->SetField("name", road->name.c_str());
            feature->SetField("osm_id", road->osm_id.c_str());

            if (layer_vehicle->CreateFeature(feature) != OGRERR_NONE) {
                cerr << "Failed to create ways feature." << endl;
            }
            OGRFeature::DestroyFeature(feature);
        }
    }

    void insert_intersect(Location location, const char *roadnames) {
        OGRFeature *feature;
        feature = OGRFeature::CreateFeature(layer_intersects->GetLayerDefn());
        
        OGRPoint *point = ogr_factory.create_point(location).release();
        if (feature->SetGeometry(point) != OGRERR_NONE) {
            cerr << "Failed to create geometry feature for intersects: ";
        }
        feature->SetField("roads", roadnames);

        if (layer_intersects->CreateFeature(feature) != OGRERR_NONE) {
            cerr << "Failed to create ways feature." << endl;
        }
        OGRFeature::DestroyFeature(feature);
    }

    void insert_node(Location location, object_id_type osm_id,
        const char *ori, double angle) {
        OGRFeature *feature;
        feature = OGRFeature::CreateFeature(layer_nodes->GetLayerDefn());
        
        OGRPoint *point = ogr_factory.create_point(location).release();
        if (feature->SetGeometry(point) != OGRERR_NONE) {
            cerr << "Failed to create geometry feature for way: ";
            cerr << osm_id << endl;
        }
        feature->SetField("osm_id", to_string(osm_id).c_str());
        feature->SetField("orientations", ori);
        feature->SetField("angle", angle);

        if (layer_nodes->CreateFeature(feature) != OGRERR_NONE) {
            cerr << "Failed to create ways feature." << endl;
        }
        OGRFeature::DestroyFeature(feature);
    }

    void union_pedestrian_geometries() {
        geos_pedestrian_net = go.union_geometries(pedestrian_geometries);
    }

    void union_vehicle_geometries() {
        geos_vehicle_net = go.union_geometries(vehicle_geometries);
    }

    void union_sidewalk_geometries() {
        geos_sidewalk_net = go.union_geometries(sidewalk_geometries);
    }

    void insert_sidewalk(OGRGeometry* sidewalk) {
        OGRFeature* feature;
        feature = OGRFeature::CreateFeature(layer_sidewalks->GetLayerDefn());
        if (feature->SetGeometry(sidewalk) != OGRERR_NONE) {
            cerr << "Failed to create geometry feature for sidewalk.";
        }
        if (layer_sidewalks->CreateFeature(feature) != OGRERR_NONE) {
            cerr << "Failed to create ways feature." << endl;
        }
        OGRFeature::DestroyFeature(feature);
    }

    void insert_in_node_map(object_id_type start_node,
            object_id_type end_node,
            VehicleRoad* road) {
	
        node_map[start_node].push_back(pair<object_id_type, VehicleRoad*>
		(end_node, road));
        node_map[end_node].push_back(pair<object_id_type, VehicleRoad*>
		(start_node, road));
        if (node_map[start_node].size() > 2) {
            order_clockwise(start_node);
        }
    }
};

#endif /* DATASTORAGE_HPP_ */