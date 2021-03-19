/* detector_manager.cc
 *
 * Copyright (C) 2011 Xavier Garrido <garrido@lal.in2p3.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

// This project:
#include <EventBrowser/detector/detector_manager.h>
#include <EventBrowser/detector/i_root_volume.h>

#include <EventBrowser/detector/box_volume.h>
#include <EventBrowser/detector/composite_volume.h>
#include <EventBrowser/detector/cylinder_volume.h>
#include <EventBrowser/detector/polycone_volume.h>
#include <EventBrowser/detector/special_volume.h>
#include <EventBrowser/detector/sphere_volume.h>
#include <EventBrowser/detector/tube_volume.h>

#include <EventBrowser/view/options_manager.h>
#include <EventBrowser/view/style_manager.h>

#include <EventBrowser/utils/root_utilities.h>

// - Bayeux/geomtools:
#include <bayeux/geomtools/box.h>
#include <bayeux/geomtools/manager.h>
#include <bayeux/geomtools/smart_id_locator.h>

// - Falaise:
#include <falaise/resource.h>

// Standard libraries:
#include <sstream>
#include <string>

// ROOT
#include <TError.h>
#include <TGeoManager.h>
#include <TGeoMatrix.h>

namespace snemo {

namespace visualization {

namespace detector {

// ctor:
detector_manager::detector_manager() {
  _initialized_ = false;
  _constructed_ = false;
  _setup_label_name_ = "";
  _setup_label_ = setup::SNEMO;
  _has_external_geometry_manager_ = false;
  _geo_manager_config_file_ = "";
  _geo_manager_ = nullptr;
  _world_volume_ = nullptr;
  _world_length_ = 0.0;
  _world_width_ = 0.0;
  _world_height_ = 0.0;
}

// dtor:
detector_manager::~detector_manager() { this->reset(); }

void detector_manager::configure(const std::string &geo_manager_config_file_) {
  this->reset();
  this->_at_init_(geo_manager_config_file_);
  _initialized_ = true;
  this->_at_construct_();
  _constructed_ = true;
}

void detector_manager::update() {
  for (const auto &it_volume : _volumes_) {
    (it_volume.second)->update();
  }
}

void detector_manager::reset() {
  if (!_has_external_geometry_manager_) {
    delete _geo_manager_;
    _geo_manager_ = nullptr;
  }

  delete gGeoManager;

  _volumes_.clear();
  _special_volume_name_.clear();

  _constructed_ = false;
  _initialized_ = false;
}

bool detector_manager::is_initialized() const { return _initialized_; }

bool detector_manager::is_constructed() const { return _constructed_; }

detector_manager::setup detector_manager::get_setup_label() const { return _setup_label_; }

const std::string &detector_manager::get_setup_name() const { return _setup_label_name_; }

bool detector_manager::is_special_volume(const std::string &volume_name_) const {
  auto name_iter =
      std::find(_special_volume_name_.begin(), _special_volume_name_.end(), volume_name_);
  return (name_iter != _special_volume_name_.end());
}

i_volume *detector_manager::get_volume(const geomtools::geom_id &id_) {
  i_volume *volume = nullptr;
  auto volume_iter = _volumes_.find(id_);

  if (volume_iter != _volumes_.end()) {
    volume = volume_iter->second;
  }
  return volume;
}

const i_volume *detector_manager::get_volume(const geomtools::geom_id &id_) const {
  i_volume *volume = nullptr;
  auto volume_iter = _volumes_.find(id_);

  if (volume_iter != _volumes_.end()) {
    volume = volume_iter->second;
  }
  return volume;
}

std::vector<geomtools::geom_id> detector_manager::get_matching_ids(
    const geomtools::geom_id &id_) const {
  std::vector<geomtools::geom_id> vids_;

  for (const auto &_volume : _volumes_) {
    const geomtools::geom_id &gid = _volume.first;
    if (geomtools::geom_id::match(gid, id_)) {
      vids_.push_back(gid);
    }
  }
  return vids_;
}

std::string detector_manager::get_volume_name(const geomtools::geom_id &id_) const {
  return _geo_manager_->get_id_mgr().id_to_human_readable_format_string(id_);
}

std::string detector_manager::get_volume_category(const geomtools::geom_id &id_) const {
  const geomtools::id_mgr &mgr = _geo_manager_->get_id_mgr();
  std::string volume_category;

  if (mgr.has_category_info(id_.get_type())) {
    const geomtools::id_mgr::category_info &c_info = mgr.get_category_info(id_.get_type());
    volume_category = c_info.get_category();
  }
  return volume_category;
}

void detector_manager::set_external_geometry_manager(geomtools::manager &geometry_manager_) {
  _has_external_geometry_manager_ = true;
  _geo_manager_ = &geometry_manager_;
}

geomtools::manager &detector_manager::get_geometry_manager() { return *_geo_manager_; }

const geomtools::manager &detector_manager::get_geometry_manager() const { return *_geo_manager_; }

TGeoVolume *detector_manager::get_world_volume() { return _world_volume_; }

const TGeoVolume *detector_manager::get_world_volume() const { return _world_volume_; }

void detector_manager::draw() {
  DT_THROW_IF(!is_initialized(), std::logic_error, "Not initialized !");
  DT_THROW_IF(!is_constructed(), std::logic_error, "Not constructed !");

  TGeoVolume *world_volume = this->get_world_volume();
  world_volume->Draw();

  // Special treatment for special volumes
  for (const auto &it_volume : _volumes_) {
    const i_volume &a_volume = *(it_volume.second);

    // Discard other type of volume
    if (a_volume.get_type() != "special") {
      continue;
    }

    // ROOT volumes
    const auto &a_special_volume = dynamic_cast<const special_volume &>(a_volume);
    a_special_volume.draw();
  }
}

void detector_manager::compute_world_coordinates(const geomtools::vector_3d &mother_pos_,
                                                 geomtools::vector_3d &world_pos_) const {
  if (_module_placement_ == nullptr) {
    DT_LOG_WARNING(view::options_manager::get_instance().get_logging_priority(),
                   "No 'module' placement setup !");
    world_pos_ = mother_pos_;
  } else {
    _module_placement_->child_to_mother(mother_pos_, world_pos_);
  }
}

void detector_manager::_at_init_(const std::string &geo_manager_config_file_) {
  // TGeoXXX stuff for ROOT
  // reduce crazy TGeo output
  gErrorIgnoreLevel = kError;

  // Instantiate TGeoManager here and then use static pointer gGeoManager
  new TGeoManager("ROOT TGeo Manager", "ROOT TGeo Manager");

  // Setting geometry manager file
  if (_has_external_geometry_manager_) {
    _setup_label_name_ = _geo_manager_->get_setup_label();
  } else {
    _geo_manager_ = new geomtools::manager;

    if (!geo_manager_config_file_.empty()) {
      _geo_manager_config_file_ = geo_manager_config_file_;
    } else {
      const std::string &geo_manager_config_file =
          view::options_manager::get_instance().get_detector_config_file();

      if (!geo_manager_config_file.empty()) {
        _geo_manager_config_file_ = geo_manager_config_file;
      } else {
        _geo_manager_config_file_ = "urn:snemo:demonstrator:geometry:4.0";
        DT_LOG_NOTICE(
            view::options_manager::get_instance().get_logging_priority(),
            "Use default SuperNEMO/demonstrator config tag : " << _geo_manager_config_file_);
      }
    }
  }
  this->_read_detector_config_();
}

void detector_manager::_at_construct_() {
  // set maximum dimension to world volume
  this->_set_world_dimensions_();

  // building world volume by ROOT
  _world_volume_ = gGeoManager->MakeBox("World Volume", nullptr, _world_length_ / 2.,
                                        _world_width_ / 2., _world_height_ / 2.);
  _world_volume_->SetVisibility(false);

  // setting world volume for ROOT Geo Manager
  gGeoManager->SetTopVolume(_world_volume_);

  // adding each volume to the world one
  this->_add_volumes_();

  // closing geometry to not to modify it later
  gGeoManager->CloseGeometry();
}

void detector_manager::_read_detector_config_() {
  // Geometry manager property config
  datatools::properties gmanager_config;

  if (_has_external_geometry_manager_) {
    const std::string gmanager_config_file = _geo_manager_config_file_;

    // Load properties from the configuration file
    uint32_t read_config_flags = datatools::properties::config::RESOLVE_PATH;
    datatools::properties::read_config(gmanager_config_file, gmanager_config, read_config_flags);

    // Prepare mapping configuration with a limited set of geo. categories
    gmanager_config.update("build_mapping", true);

    // Remove the 'exclusion' property if any
    if (gmanager_config.has_key("mapping.excluded_categories")) {
      gmanager_config.erase("mapping.excluded_categories");
    }

    // Get setup label (snemo, test bench, bipo ...)
    _setup_label_name_ = gmanager_config.fetch_string("setup_label");
  }

  if (_setup_label_name_ == "snemo") {
    _setup_label_ = setup::SNEMO;
  } else if (_setup_label_name_ == "test_bench") {
    _setup_label_ = setup::TEST_BENCH;
  } else if (_setup_label_name_ == "bipo1") {
    _setup_label_ = setup::BIPO1;
  } else if (_setup_label_name_ == "bipo3") {
    _setup_label_ = setup::BIPO3;
    _special_volume_name_.emplace_back("light_guide.category");
  } else if (_setup_label_name_ == "snemo::tracker_commissioning") {
    _setup_label_ = setup::TRACKER_COMMISSIONING;
  } else if (_setup_label_name_ == "snemo::demonstrator") {
    _setup_label_ = setup::SNEMO_DEMONSTRATOR;
  } else if (_setup_label_name_ == "hpge") {
    _setup_label_ = setup::HPGE;
  } else {
    DT_LOG_WARNING(
        view::options_manager::get_instance().get_logging_priority(),
        "Geometry setup with label '" << _setup_label_name_ << "' is not natively supported !");
    _setup_label_ = setup::UNDEFINED;
  }

  // Load the given style file in order to get the activated detectors
  view::style_manager &style_mgr = view::style_manager::get_instance();
  style_mgr.set_setup_label(_setup_label_name_);
  const std::string &style_config_file =
      view::options_manager::get_instance().get_style_config_file();
  style_mgr.configure(style_config_file);

  // Get volume categories
  std::vector<std::string> only_categories;
  this->_set_categories_(only_categories);

  // Initialize geometry manager
  geomtools::manager &gmgr = this->get_geometry_manager();
  gmgr.set_logging_priority(view::options_manager::get_instance().get_logging_priority());
  if (_has_external_geometry_manager_) {
    // Set the 'only' property
    // gmanager_config.update("mapping.only_categories", only_categories);
    gmgr.initialize(gmanager_config);
  }

  // If no categories has been set then add all category from id manager
  // to the list of detector volume
  if (only_categories.empty()) {
    std::map<std::string, view::style_manager::volume_properties> &volumes =
        style_mgr.get_volumes_properties();

    const geomtools::id_mgr::categories_by_name_col_type &categories =
        gmgr.get_id_mgr().categories_by_name();
    for (const auto &categorie : categories) {
      const std::string &a_category = categorie.first;
      only_categories.push_back(a_category);
      view::style_manager::volume_properties dummy;
      volumes[a_category] = dummy;
      volumes[a_category]._visibility_ = VISIBLE;
      volumes[a_category]._color_ = utils::root_utilities::get_random_color();
    }
  }

  for (const std::string &volume_category_name : only_categories) {
    const geomtools::mapping &mapping = gmgr.get_mapping();
    const auto &categories = gmgr.get_id_mgr().categories_by_name();

    auto found = categories.find(volume_category_name);
    if (found == categories.end()) {
      DT_LOG_WARNING(
          view::options_manager::get_instance().get_logging_priority(),
          "Category '" << volume_category_name << "' is not mapped ! "
                       << "Check categories.lis file to address the right category name");
      continue;
    }

    const int volume_type = found->second.get_type();

    geomtools::smart_id_locator volume_locator;
    volume_locator.set_gmap(mapping);
    volume_locator.initialize(volume_type);

    const std::list<const geomtools::geom_info *> &geom_info_list = volume_locator.get_ginfos();

    if (geom_info_list.empty()) {
      DT_LOG_WARNING(view::options_manager::get_instance().get_logging_priority(),
                     "No geom info for '" << volume_category_name << "' "
                                          << "has been associated with the geometry map !");
    }

    for (auto iptr_ginfo : geom_info_list) {
      const geomtools::geom_info &ginfo = *iptr_ginfo;
      this->_set_volume_(ginfo);

      // Save 'module' placement for later coordinates conversion
      if (volume_category_name == "module") {
        _module_placement_ = &(ginfo.get_world_placement());
      }
    }  // end of geo_info
  }    // end of enabled categories
}

void detector_manager::_set_categories_(std::vector<std::string> &only_categories_) const {
  view::style_manager &style_mgr = view::style_manager::get_instance();
  auto &volumes = style_mgr.get_volumes_properties();

  // first get them from style file
  if (!volumes.empty()) {
    for (const auto &it_volume : volumes) {
      if ((it_volume.second)._visibility_ == DISABLE) {
        continue;
      }
      DT_LOG_DEBUG(view::options_manager::get_instance().get_logging_priority(),
                   "Add '" << it_volume.first << "' category");
      only_categories_.push_back(it_volume.first);
    }

    // special treatment in case of all volumes disabled
    if (only_categories_.empty()) {
      switch (_setup_label_) {
        case setup::SNEMO:
        case setup::SNEMO_DEMONSTRATOR:
        case setup::TRACKER_COMMISSIONING:
          only_categories_.emplace_back("module");
          volumes["module"]._visibility_ = VISIBLE;
          break;
        case setup::TEST_BENCH:
          only_categories_.emplace_back("calo_light_line");
          volumes["calo_light_line"]._visibility_ = VISIBLE;
          break;
        default:
          break;
      }
    }
  } else {
    // if no volume are setup in style file
    // then it behaves like the following
    switch (_setup_label_) {
      case setup::TRACKER_COMMISSIONING:
        only_categories_.emplace_back("hall");
        only_categories_.emplace_back("module");
        only_categories_.emplace_back("mu_trigger");
        only_categories_.emplace_back("drift_cell_core");
        break;
      case setup::SNEMO:
      case setup::SNEMO_DEMONSTRATOR:
        only_categories_.emplace_back("hall");
        // only_categories_.push_back("external_shield");
        only_categories_.emplace_back("module");
        // only_categories_.push_back("source_submodule");
        only_categories_.emplace_back("source_strip");
        only_categories_.emplace_back("source_pad");
        // only_categories_.push_back("source_calibration_track");
        // only_categories_.push_back("source_calibration_carrier");
        only_categories_.emplace_back("source_calibration_spot");
        only_categories_.emplace_back("source_like_plate");
        only_categories_.emplace_back("commissioning_source_spot");
        only_categories_.emplace_back("drift_cell_core");
        only_categories_.emplace_back("xcalo_block");
        only_categories_.emplace_back("gveto_block");
        only_categories_.emplace_back("calorimeter_block");
        break;
      case setup::TEST_BENCH:
        only_categories_.emplace_back("calo_light_line");
        only_categories_.emplace_back("scin_block");
        only_categories_.emplace_back("absorber");
        only_categories_.emplace_back("diaphragm");
        only_categories_.emplace_back("air_gap");
        only_categories_.emplace_back("test_source");
        break;
      case setup::BIPO1:
      case setup::BIPO3:
        only_categories_.emplace_back("scin_block");
        only_categories_.emplace_back("source");
        //            only_categories_.push_back("bipo3_module");
        break;
      default:
        break;
    }

    // adding the default detectors to detector_to_show list
    for (const auto &it_category : only_categories_) {
      volumes[it_category]._visibility_ = VISIBLE;
    }
  }
}

void detector_manager::_set_volume_(const geomtools::geom_info &ginfo_) {
  const geomtools::geom_id &volume_id = ginfo_.get_id();

  const std::string volume_name = this->get_volume_name(volume_id);
  const std::string volume_category = this->get_volume_category(volume_id);

  const geomtools::logical_volume &a_log = ginfo_.get_logical();
  const geomtools::i_shape_3d &a_shape = a_log.get_shape();
  const std::string &shape_name = a_shape.get_shape_name();

  if (is_special_volume(volume_category)) {
    _volumes_[volume_id] = new special_volume(volume_name, volume_category);
  } else if (a_shape.is_composite()) {
    _volumes_[volume_id] = new composite_volume(volume_name, volume_category);
  } else if (shape_name == "box") {
    _volumes_[volume_id] = new box_volume(volume_name, volume_category);
  } else if (shape_name == "cylinder") {
    _volumes_[volume_id] = new cylinder_volume(volume_name, volume_category);
  } else if (shape_name == "tube") {
    _volumes_[volume_id] = new tube_volume(volume_name, volume_category);
  } else if (shape_name == "sphere") {
    _volumes_[volume_id] = new sphere_volume(volume_name, volume_category);
  } else if (shape_name == "polycone") {
    _volumes_[volume_id] = new polycone_volume(volume_name, volume_category);
  } else {
    DT_LOG_ERROR(view::options_manager::get_instance().get_logging_priority(),
                 shape_name << "' not yet implemented for '" << volume_category << "' volume");
    return;
  }

  _volumes_[volume_id]->initialize(ginfo_);
  _volumes_[volume_id]->update();
}

void detector_manager::_add_volumes_() {
  for (auto &_volume : _volumes_) {
    i_volume &a_volume = *(_volume.second);

    // Discard special volume
    if (a_volume.get_type() == "special") {
      continue;
    }

    const geomtools::placement &placement = a_volume.get_placement();
    const geomtools::vector_3d &position = placement.get_translation();
    TGeoTranslation translation(position.x(), position.y(), position.z());

    TGeoRotation rotation((std::string("rot") + a_volume.get_name()).c_str(),
                          placement.get_phi() / CLHEP::degree + 90.0,
                          placement.get_theta() / CLHEP::degree,
                          placement.get_delta() / CLHEP::degree + 90.0);

    // ROOT volumes
    auto &a_root_volume = dynamic_cast<i_root_volume &>(a_volume);
    auto *gvolume = static_cast<TGeoVolume *>(a_root_volume.grab_volume());

    // sanity check
    DT_THROW_IF(
        !gvolume, std::logic_error,
        "No TGeoVolume has been constructed for '" << a_root_volume.get_name() << "' volume !");

    // The second parameter is related to replication of volume:
    // 1 means that all volume are unique which is true but most of them
    // have actually identical shape. Probably this needs improvements in future.
    // To be continued ...
    _world_volume_->AddNodeOverlap(gvolume, 1, new TGeoCombiTrans(translation, rotation));
  }
}

void detector_manager::_set_world_dimensions_() {
  const geomtools::manager &gmgr = this->get_geometry_manager();
  const geomtools::mapping &mapping = gmgr.get_mapping();
  const geomtools::id_mgr::categories_by_name_col_type &categories =
      gmgr.get_id_mgr().categories_by_name();

  const int volume_type = categories.find("world")->second.get_type();
  geomtools::smart_id_locator volume_locator;
  volume_locator.set_gmap(mapping);
  volume_locator.initialize(volume_type);

  for (auto iptr_ginfo : volume_locator.get_ginfos()) {
    const geomtools::geom_info &ginfo = *iptr_ginfo;

    const geomtools::logical_volume &a_log = ginfo.get_logical();
    const geomtools::i_shape_3d &a_shape = a_log.get_shape();

    const std::string &shape_name = a_shape.get_shape_name();

    if (shape_name == "box") {
      const auto &mbox = dynamic_cast<const geomtools::box &>(a_shape);

      _world_length_ = mbox.get_x();
      _world_width_ = mbox.get_y();
      _world_height_ = mbox.get_z();
    } else {
      DT_LOG_ERROR(view::options_manager::get_instance().get_logging_priority(),
                   "World volume only support box shape "
                       << "and nothing else. Do not do crazy thing buddy !");
    }
  }

  const double boundary = 1.1;

  _world_length_ *= boundary;
  _world_width_ *= boundary;
  _world_height_ *= boundary;
}

}  // end of namespace detector

}  // end of namespace visualization

}  // end of namespace snemo

// end of detector_manager.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
