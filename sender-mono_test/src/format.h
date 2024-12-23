// Create the json sent to the clients
nlohmann::json toJSON(int frame_id, int serial_number, sl::Timestamp timestamp, sl::Pose& cam_pose, sl::COORDINATE_SYSTEM coord_sys, sl::UNIT coord_unit)
{
    nlohmann::json j;

    j["serial_number"] = serial_number;
    j["frame_id"] = frame_id;
    j["timestamp"] = timestamp.data_ns;
    j["role"] = ZEDLiveLinkRole::Camera;
    j["coordinate_system"] = coord_sys;
    j["coordinate_unit"] = coord_unit;

    j["camera_position"] = nlohmann::json::object();
    j["camera_position"]["x"] = isnan(cam_pose.getTranslation().x) ? 0 : cam_pose.getTranslation().x;
    j["camera_position"]["y"] = isnan(cam_pose.getTranslation().y) ? 0 : cam_pose.getTranslation().y;
    j["camera_position"]["z"] = isnan(cam_pose.getTranslation().z) ? 0 : cam_pose.getTranslation().z;

    j["camera_orientation"] = nlohmann::json::object();
    j["camera_orientation"]["x"] = isnan(cam_pose.getOrientation().x) ? 0 : cam_pose.getOrientation().x;
    j["camera_orientation"]["y"] = isnan(cam_pose.getOrientation().x) ? 0 : cam_pose.getOrientation().y;
    j["camera_orientation"]["z"] = isnan(cam_pose.getOrientation().x) ? 0 : cam_pose.getOrientation().z;
    j["camera_orientation"]["w"] = isnan(cam_pose.getOrientation().x) ? 0 : cam_pose.getOrientation().w;

    return j;
}


// send one skeleton at a time
nlohmann::json toJSON(int frame_id, sl::Timestamp timestamp, sl::Bodies& bodies, int id, sl::BODY_FORMAT body_format, sl::COORDINATE_SYSTEM coord_sys, sl::UNIT coord_unit)
{
    nlohmann::json j;

    j["frame_id"] = frame_id;
    j["timestamp"] = timestamp.data_ns;
    j["role"] = ZEDLiveLinkRole::Animation;
    j["body_format"] = body_format;
    j["is_new"] = (bool)bodies.is_new;
    j["coordinate_system"] = coord_sys;
    j["coordinate_unit"] = coord_unit;
    j["nb_bodies"] = bodies.body_list.size();

    if (id < bodies.body_list.size())
    {
        auto body = bodies.body_list[id];

        j["tracking_state"] = (int)body.tracking_state;
        j["action_state"] = (int)body.action_state;
        j["id"] = body.id;
        j["position"] = nlohmann::json::object();
        j["position"]["x"] = isnan(body.position.x) ? 0 : body.position.x;
        j["position"]["y"] = isnan(body.position.y) ? 0 : body.position.y;
        j["position"]["z"] = isnan(body.position.z) ? 0 : body.position.z;

        j["confidence"] = isnan(body.confidence) ? 0 : body.confidence;

        j["keypoint_confidence"] = nlohmann::json::array();
        for (auto& i : body.keypoint_confidence)
        {
            j["keypoint_confidence"].push_back(isnan(i) ? 0 : i);
        }

        j["keypoint"] = nlohmann::json::array();
        for (auto& i : body.keypoint)
        {
            nlohmann::json e;
            e["x"] = isnan(i.x) ? 0 : i.x;
            e["y"] = isnan(i.y) ? 0 : i.y;
            e["z"] = isnan(i.z) ? 0 : i.z;
            j["keypoint"].push_back(e);
        }
        j["local_position_per_joint"] = nlohmann::json::array();
        for (auto& i : body.local_position_per_joint)
        {
            nlohmann::json e;
            e["x"] = isnan(i.x) ? 0 : i.x;
            e["y"] = isnan(i.y) ? 0 : i.y;
            e["z"] = isnan(i.z) ? 0 : i.z;
            j["local_position_per_joint"].push_back(e);
        }
        j["local_orientation_per_joint"] = nlohmann::json::array();
        for (auto& i : body.local_orientation_per_joint)
        {
            nlohmann::json e;
            e["x"] = isnan(i.x) ? 0 : i.x;
            e["y"] = isnan(i.y) ? 0 : i.y;
            e["z"] = isnan(i.z) ? 0 : i.z;
            e["w"] = isnan(i.w) ? 0 : i.w;
            j["local_orientation_per_joint"].push_back(e);
        }

        j["global_root_posititon"] = nlohmann::json::object();
        j["global_root_posititon"]["x"] = isnan(body.keypoint[0].x) ? 0 : body.keypoint[0].x;
        j["global_root_posititon"]["y"] = isnan(body.keypoint[0].y) ? 0 : body.keypoint[0].y;
        j["global_root_posititon"]["z"] = isnan(body.keypoint[0].z) ? 0 : body.keypoint[0].z;

        j["global_root_orientation"] = nlohmann::json::object();
        j["global_root_orientation"]["x"] = isnan(body.global_root_orientation.x) ? 0 : body.global_root_orientation.x;
        j["global_root_orientation"]["y"] = isnan(body.global_root_orientation.y) ? 0 : body.global_root_orientation.y;
        j["global_root_orientation"]["z"] = isnan(body.global_root_orientation.z) ? 0 : body.global_root_orientation.z;
        j["global_root_orientation"]["w"] = isnan(body.global_root_orientation.w) ? 0 : body.global_root_orientation.w;
    }

    return j;
}