/*
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include <glog/logging.h>

#include <chrono>
#include <future>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <sstream>
#include <thread>

#include "src/primihub/common/config/config.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/service/dataset/util.hpp"
#include "src/primihub/util/redis_helper.h"

using namespace std::chrono_literals;

namespace primihub::service {

DatasetService::DatasetService(std::shared_ptr<DatasetMetaService> metaService,
                               const std::string &nodelet_addr) {
  metaService_ = metaService;
  nodelet_addr_ = nodelet_addr;
  // create Flight server
  // flight_server_ =
  // std::make_shared<FlightIntegrationServer>(shared_from_this());
  //
}

/**
 * @brief Construct a new Dataset object
 * 1. Read data using driver for get dataset & datameta
 * 2. Save datameta in local storage.
 * 3. Publish dataset meta on libp2p network.
 *
 * @param driver [input]: Data driver
 * @param name [input]: Dataset name
 * @param description [input]: Dataset description
 * @param meta [output]: Dataset meta
 * @return std::shared_ptr<primihub::Dataset>
 */
std::shared_ptr<primihub::Dataset> DatasetService::newDataset(
    std::shared_ptr<primihub::DataDriver> driver,
    const std::string &description, // TODO put description in meta
    DatasetMeta &meta) {

  // Read data using driver for get dataset & datameta
  auto dataset = driver->getCursor()->read();
  DatasetMeta _meta(dataset, description,
                    DatasetVisbility::PUBLIC); // TODO(chenhongbo) visibility
                                               // public for test now.
  meta = _meta;
  // Save datameta in local storage.& Publish dataset meta on libp2p network.
  metaService_->putMeta(meta);
  return dataset;
}

/**
 * @brief write dataset to local storage
 * @param dataset [input]: Dataset to be written with own driver
 * @param description [input]: Dataset description
 * @param meta [output]: Dataset metadata
 */
void DatasetService::writeDataset(
    const std::shared_ptr<primihub::Dataset> &dataset,
    const std::string &description, DatasetMeta &meta /*output*/) {
  // dataset.write();
  dataset->getDataDriver()->getCursor()->write(dataset);
  DatasetMeta _meta(dataset, description,
                    DatasetVisbility::PUBLIC); // TODO(chenhongbo) visibility
                                               // public for test now.
  meta = _meta;
  metaService_->putMeta(meta);
}

/**
 * @brief Register dataset use meta
 *
 * @param meta [input]: Dataset meta
 */
void DatasetService::regDataset(DatasetMeta &meta) {
  metaService_->putMeta(meta);
}

/**
 * @brief Read dataset by dataset uri
 * TODO(chenhongbo) Only support local dataset now. !!!
 * @param id [input]: Dataset id
 * @param handler [input]: Read dataset handler
 * @return outcome::result<void>
 *  TODO async style or callback style ?
 */
outcome::result<void> DatasetService::readDataset(const DatasetId &id,
                                                  ReadDatasetHandler handler) {
  metaService_->getMeta(id, [&](std::shared_ptr<DatasetMeta> meta) {
    if (meta) {
      // Construct dataset from meta
      auto driver =
          DataDirverFactory::getDriver(meta->getDriverType(), nodelet_addr_);
      auto cursor = driver->read(
          meta->getDataURL()); // TODO only support Local file path now.
      auto dataset = cursor->read();
      handler(dataset);
      return outcome::success();
    }
  });
  return outcome::success();
}

/**
 * @brief Find dataset by dataset uri
 *
 */
outcome::result<void> DatasetService::findDataset(const DatasetId &id,
                                                  FoundMetaHandler handler) {
  metaService_->getMeta(id, handler);
  return outcome::success();
}

/**
 * @brief TODO Delete dataset
 * 1. Delete dataset meta from local storage.
 * 2. Delete dataset from libp2p network.
 *
 * @param id [input]: Dataset id
 * @return int
 */
int DatasetService::deleteDataset(const DatasetId &id) { return 0; }

/**
 * @brief Consture datasets from yaml datasets configurtion.
 * @param config_file_path [input]: Datasets configuration file path
 *
 */
void DatasetService::loadDefaultDatasets(const std::string &config_file_path) {
  LOG(INFO) << "📃 Load default datasets from config: " << config_file_path;
  YAML::Node config = YAML::LoadFile(config_file_path);
  // strcat nodelet address
  std::string nodelet_addr = config["node"].as<std::string>() + ":" +
                             config["location"].as<std::string>() + ":" +
                             std::to_string(config["grpc_port"].as<uint64_t>());

  if (config["datasets"]) {
    for (auto dataset : config["datasets"]) {
      auto driver = DataDirverFactory::getDriver(
          std::move(dataset["model"].as<std::string>()), nodelet_addr);

      auto source = dataset["source"].as<std::string>();
      [[maybe_unused]] auto cursor = driver->read(source);
      DatasetMeta meta;
      newDataset(driver, dataset["description"].as<std::string>(), meta);
    }
  }
}

// Load dataset from local meta storage.
void DatasetService::restoreDatasetFromLocalStorage(void) {
  LOG(INFO) << "💾 Restore dataset from local storage...";
  std::vector<DatasetMeta> metas;
  metaService_->getAllLocalMetas(metas);
  for (auto meta : metas) {
    // Update node let address.
    std::string node_id, node_ip, dataset_path;
    int node_port;
    std::string data_url = meta.getDataURL();
    if (!DataURLToDetail(data_url, node_id, node_ip, node_port, dataset_path)) {
      LOG(ERROR) << "💾 Restore dataset from local storage failed: " << data_url;
      continue;
    }

    meta.setDataURL(nodelet_addr_ + ":" + dataset_path);
    // Publish dataset meta on libp2p network.
    metaService_->putMeta(meta);
  }
}

void DatasetService::setMetaSearchTimeout(unsigned int timeout) {
  if (metaService_) {
    metaService_->setMetaSearchTimeout(timeout);
  }
}

std::string DatasetService::getNodeletAddr(void) { return nodelet_addr_; }

// ======================== DatasetMetaService
// ====================================
DatasetMetaService::DatasetMetaService(
    std::shared_ptr<primihub::p2p::NodeStub> p2pStub,
    std::shared_ptr<StorageBackend> localKv) {
  p2pStub_ = p2pStub;
  localKv_ = localKv;
}

// Get all local metas
outcome::result<void>
DatasetMetaService::getAllLocalMetas(std::vector<DatasetMeta> &metas) {
  // Get all k, v from local storage
  auto r = localKv_->getAll();
  auto rl = r.value();
  for (auto kv_pair : rl) {
    // Construct meta from k, v
    auto meta = DatasetMeta(kv_pair.second);
    metas.push_back(meta);
  }
  return outcome::success();
}

std::shared_ptr<DatasetMeta>
DatasetMetaService::getLocalMeta(const DatasetId &id) {
  auto res = localKv_->getValue(id);
  if (res.has_value()) {
    return std::make_shared<DatasetMeta>(res.value());
  }
  return nullptr;
}

void DatasetMetaService::putMeta(DatasetMeta &meta) {
  std::string meta_str = meta.toJSON();
  LOG(INFO) << "<< Put meta: " << meta_str;
  // Save datameta in local storage.
  localKv_->putValue(meta.id, meta_str);

  // Publish dataset meta on libp2p network.
  p2pStub_->putDHTValue(meta.id, meta_str);
}

outcome::result<void> DatasetMetaService::getMeta(const DatasetId &id,
                                                  FoundMetaHandler handler) {
  // Try get meta from local storage or p2p network
  auto res = localKv_->getValue(id);
  if (res.has_value()) {
    // construct dataset meta from json meta.
    auto meta = std::make_shared<DatasetMeta>(res.value());
    handler(meta);
    return outcome::success();
  }

  // Try get dataset from p2p network
  p2pStub_->getDHTValue(
      id,
      [&](libp2p::outcome::result<libp2p::protocol::kademlia::Value> result) {
        if (result.has_value()) {
          try {
            auto r = result.value();
            std::stringstream rs;
            std::copy(r.begin(), r.end(),
                      std::ostream_iterator<uint8_t>(rs, ""));
            std::cout << rs.str() << std::endl;
            auto _meta = std::make_shared<DatasetMeta>(std::move(rs.str()));
            handler(_meta);
          } catch (std::exception &e) {
            LOG(ERROR) << "<< Get meta failed: " << e.what();
          }
        }
      });
  return outcome::success();
}

outcome::result<void> DatasetMetaService::findPeerListFromDatasets(
    const std::vector<DatasetWithParamTag> &datasets_with_tag,
    FoundMetaListHandler handler) {
  std::vector<DatasetMetaWithParamTag> meta_list;
  std::lock_guard<std::mutex> lock(meta_map_mutex_);
  meta_map_.clear();
  auto t_start = std::chrono::high_resolution_clock::now();
  bool is_timeout = false;
  for (;;) {
    // TODO timeout guard
    auto t_now = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(t_now - t_start)
            .count() > this->meta_search_timeout_) {
      LOG(ERROR) << " 🔍 ⏱️  Timeout while searching meta list.";
      is_timeout = true;
      break;
    }

    for (auto dataset_item : datasets_with_tag) {
      auto dataset_name = std::get<0>(dataset_item);
      auto dataset_tag = std::get<1>(dataset_item);
      DatasetId id(dataset_name);
      // Try get meta from local storage or p2p network
      auto res = localKv_->getValue(id);
      if (res.has_value()) {
        // construct dataset meta from json meta.
        auto _meta = std::make_shared<DatasetMeta>(res.value());
        // meta_list.push_back(std::move(_meta));
        LOG(INFO) << "Found local meta: " << res.value();
        auto k = _meta->getDescription();
        meta_map_.insert({k, std::make_pair(_meta, dataset_tag)});

      } else {
        // Find in DHT
        p2pStub_->getDHTValue(
            id, [&, dataset_tag](
                    libp2p::outcome::result<libp2p::protocol::kademlia::Value>
                        result) {
              if (result.has_value()) {
                try {
                  std::vector<std::shared_ptr<DatasetMeta>> meta_list;
                  auto r = result.value();
                  std::stringstream rs;
                  std::copy(r.begin(), r.end(),
                            std::ostream_iterator<uint8_t>(rs, ""));
                  LOG(INFO) << "Fount remote meta: " << rs.str();
                  auto _meta =
                      std::make_shared<DatasetMeta>(std::move(rs.str()));
                  auto k = _meta->getDescription();
                  meta_map_.insert({k, std::make_pair(_meta, dataset_tag)});
                } catch (std::exception &e) {
                  LOG(ERROR) << "<< Get meta failed: " << e.what();
                }
              }
            });
      }
    }
    if (meta_map_.size() < datasets_with_tag.size()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    } else {
      break;
    }
  }
  if (is_timeout) {
    return outcome::success();
  }
  for (auto &meta : meta_map_) {
    meta_list.push_back(std::make_pair(meta.second.first, meta.second.second));
  }
  handler(meta_list);
  return outcome::success();
}

// Implement of CentralizedDatasetMetaService.
CentralizedDatasetMetaService::CentralizedDatasetMetaService(
    std::string &service_addr,
    std::shared_ptr<primihub::service::StorageBackend> local_db,
    std::shared_ptr<primihub::p2p::NodeStub> dummy)
    : DatasetMetaService(dummy, local_db) {
  service_addr_ = service_addr;
  local_db_ = local_db;
  channel_ = grpc::CreateChannel(this->service_addr_,
                                 grpc::InsecureChannelCredentials());
  stub_ = CentralizedDatasetService::NewStub(channel_);
}

void CentralizedDatasetMetaService::putMeta(DatasetMeta &meta) {
  std::string meta_str = meta.toJSON();
  LOG(INFO) << "<< Put meta to centralized dataset meta service, meta "
            << meta_str;

  auto dataset_id = libp2p::multi::ContentIdentifierCodec::toString(
      libp2p::multi::ContentIdentifierCodec::decode(meta.id.data).value());

  RegDatasetResponse response;
  RegDatasetRequest request;
  request.set_name(dataset_id.value());
  request.set_dataset_url(meta_str);

  grpc::ClientContext context;
  grpc::Status status = stub_->RegDataset(&context, request, &response);
  if (!status.ok()) {
    LOG(ERROR) << "Issue RegDataset failed, " << status.error_message();
    return;
  }

  if (response.status() != RequestStatus::FINISH) {
    LOG(ERROR) << "Register dataset failed, " << response.msg() << ".";
    return;
  }

  local_db_->putValue(meta.id, meta_str);
  LOG(INFO) << "Register dataset " << meta.getDescription()
            << " to remote service finish.";
}

outcome::result<void>
CentralizedDatasetMetaService::getMeta(const DatasetId &id,
                                       FoundMetaHandler handler) {
  auto dataset_id = libp2p::multi::ContentIdentifierCodec::toString(
      libp2p::multi::ContentIdentifierCodec::decode(id.data).value());

  FindDatasetRequest request;
  FindDatasetResponse response;

  request.set_name(dataset_id.value());

  grpc::ClientContext context;
  grpc::Status status = stub_->FindDataset(&context, request, &response);
  if (!status.ok()) {
    LOG(ERROR) << "Issue FindDataset failed, " << status.error_message();
    return outcome::success();
  }

  if (response.status() != RequestStatus::FINISH) {
    LOG(ERROR) << "Find dataset failed, " << response.msg() << ".";
    return outcome::success();
  }

  std::shared_ptr<DatasetMeta> meta =
      std::make_shared<DatasetMeta>(std::move(response.msg()));
  handler(meta);

  LOG(INFO) << "Get meta with dataset id " << dataset_id.value() << " finish.";
  return outcome::success();
}

outcome::result<void> CentralizedDatasetMetaService::findPeerListFromDatasets(
    const std::vector<DatasetWithParamTag> &datasets_with_tag,
    FoundMetaListHandler handler) {
  std::vector<DatasetMetaWithParamTag> meta_list;

  for (auto pair : datasets_with_tag) {
    auto dataset_name = std::get<0>(pair);
    auto dataset_tag = std::get<1>(pair);

    DatasetId id(dataset_name);
    auto dataset_id = libp2p::multi::ContentIdentifierCodec::toString(
        libp2p::multi::ContentIdentifierCodec::decode(id.data).value());

    FindDatasetRequest request;
    FindDatasetResponse response;
    grpc::ClientContext context;

    request.set_name(dataset_id.value());
    grpc::Status status = stub_->FindDataset(&context, request, &response);
    if (!status.ok()) {
      LOG(ERROR) << "Issue FindDataset failed, " << status.error_message();
      LOG(ERROR) << "findPeerListFromDatasets quit due to error.";
      break;
    }

    if (response.status() != RequestStatus::FINISH) {
      LOG(ERROR) << "Find dataset failed, " << response.msg() << ".";
      LOG(ERROR) << "findPeerListFromDatasets quit due to error.";
      break;
    }

    std::shared_ptr<DatasetMeta> meta =
        std::make_shared<DatasetMeta>(std::move(response.msg()));
    meta_list.emplace_back(meta, dataset_tag);

    LOG(INFO) << "Get dataset meta for dataset " << dataset_name
              << " from centralized service finish.";
  }

  if (datasets_with_tag.size() != meta_list.size())
    LOG(ERROR) << "Failed to get all dataset's meta, no handler triggered.";
  else
    handler(meta_list);

  handler(meta_list);
  return outcome::success();
}

RedisDatasetMetaService::RedisDatasetMetaService(
    std::string &redis_addr, std::string &redis_passwd,
    std::shared_ptr<primihub::service::StorageBackend> local_db,
    std::shared_ptr<primihub::p2p::NodeStub> dummy)
    : DatasetMetaService(dummy, local_db) {
  this->redis_addr_ = redis_addr;
  this->local_db_ = local_db;
  this->redis_passwd_ = redis_passwd;
}

void RedisDatasetMetaService::putMeta(DatasetMeta &meta) {
  RedisStringKVHelper helper;
  if (helper.connect(this->redis_addr_, this->redis_passwd_)) {
    LOG(ERROR) << "Connect to redis server " << this->redis_addr_ << " failed.";
    return;
  }

  std::string meta_str = meta.toJSON();
  LOG(INFO) << "<< Put meta to redis dataset meta service, meta " << meta_str;

  auto dataset_id = libp2p::multi::ContentIdentifierCodec::toString(
      libp2p::multi::ContentIdentifierCodec::decode(meta.id.data).value());

  if (helper.setString(dataset_id.value(), meta_str)) {
    LOG(ERROR) << "Save dataset " << meta.getDescription()
               << " and it's meta to redis failed, dataset id is "
               << dataset_id.value() << ".";
    return;
  }

  local_db_->putValue(meta.id, meta_str);
  LOG(INFO) << "Save dataset " << meta.getDescription()
            << "'s meta to redis finish.";
  return;
}

outcome::result<void>
RedisDatasetMetaService::getMeta(const DatasetId &id,
                                 FoundMetaHandler handler) {
  RedisStringKVHelper helper;
  if (helper.connect(this->redis_addr_, this->redis_passwd_)) {
    LOG(ERROR) << "Connect to redis server " << this->redis_addr_ << "failed.";
    return outcome::success();
  }

  auto dataset_id = libp2p::multi::ContentIdentifierCodec::toString(
      libp2p::multi::ContentIdentifierCodec::decode(id.data).value());

  std::string meta_str;
  if (helper.getString(dataset_id.value(), meta_str)) {
    LOG(ERROR) << "Get meta for dataset id " << dataset_id.value()
               << " from redis failed.";
    return outcome::success();
  }

  std::shared_ptr<DatasetMeta> meta = std::make_shared<DatasetMeta>(meta_str);
  handler(meta);

  LOG(INFO) << "Get dataset meta with dataset id " << dataset_id.value()
            << " from redis finish.";
  return outcome::success();
}

outcome::result<void> RedisDatasetMetaService::findPeerListFromDatasets(
    const std::vector<DatasetWithParamTag> &datasets_with_tag,
    FoundMetaListHandler handler) {
  std::vector<DatasetMetaWithParamTag> meta_list;

  RedisStringKVHelper helper;
  if (helper.connect(this->redis_addr_, this->redis_passwd_)) {
    LOG(ERROR) << "Connect to redis server " << this->redis_addr_ << "failed.";
    return outcome::success();
  }

  for (auto pair : datasets_with_tag) {
    auto dataset_name = std::get<0>(pair);
    auto dataset_tag = std::get<1>(pair);

    DatasetId id(dataset_name);
    auto dataset_id = libp2p::multi::ContentIdentifierCodec::toString(
        libp2p::multi::ContentIdentifierCodec::decode(id.data).value());

    std::string meta_str;
    if (helper.getString(dataset_id.value(), meta_str)) {
      LOG(ERROR) << "Get dataset meta from redis server for dataset "
                 << dataset_name << " failed.";
      break;
    }

    std::shared_ptr<DatasetMeta> meta = std::make_shared<DatasetMeta>(meta_str);
    meta_list.emplace_back(meta, dataset_tag);

    LOG(INFO) << "Get meta for dataset " << dataset_name
              << " from redis finish.";
  }

  if (datasets_with_tag.size() != meta_list.size())
    LOG(ERROR) << "Failed to get all dataset's meta, no handler triggered.";
  else
    handler(meta_list);

  return outcome::success();
}

////////////////Flight Server ///////

arrow::Status
FlightIntegrationServer::DoGet(const arrow::flight::ServerCallContext &context,
                               const arrow::flight::Ticket &request,
                               std::unique_ptr<FlightDataStream> *data_stream) {
  // NOTE fligth ticket format : fligt.{dataset_id_str}
  DatasetId id(request.ticket);
  std::shared_ptr<DatasetMeta> meta =
      dataset_service_->metaService_->getLocalMeta(id);
  if (meta == nullptr) {
    LOG(WARNING) << "Could not find flight ticket: " << request.ticket;
    return arrow::Status::KeyError(
        "Could not find flight ticket: ",
        request.ticket); // NOTE path is dataset description
  }
  // read dataset from local driver
  auto driver = DataDirverFactory::getDriver(
      meta->getDriverType(), dataset_service_->getNodeletAddr());

  std::string node_id, node_ip, dataset_path;
  int node_port;
  std::string data_url = meta->getDataURL();
  LOG(INFO) << "DoGet dataset url:" << data_url;
  DataURLToDetail(data_url, node_id, node_ip, node_port, dataset_path);
  LOG(INFO) << "DoGet dataset path:" << dataset_path;
  auto cursor =
      driver->read(dataset_path); // TODO only support Local file path now.
  auto dataset = cursor->read();
  LOG(INFO) << "DoGet dataset read done";

  // NOTE that we can't directly pass TableBatchReader to
  // RecordBatchStream because TableBatchReader keeps a non-owning
  // reference to the underlying Table, which would then get freed
  // when we exit this function
  auto table = std::get<0>(dataset->data);
  std::vector<std::shared_ptr<arrow::RecordBatch>> batches;
  arrow::TableBatchReader batch_reader(*table);
  // ARROW_ASSIGN_OR_RAISE(batches, batch_reader.ToRecordBatches());
  batch_reader.ReadAll(&batches);

  ARROW_ASSIGN_OR_RAISE(
      auto owning_reader,
      arrow::RecordBatchReader::Make(std::move(batches), table->schema()));
  *data_stream = std::unique_ptr<arrow::flight::FlightDataStream>(
      new arrow::flight::RecordBatchStream(owning_reader));
  LOG(INFO) << "DoGet dataset send to client done";
  return arrow::Status::OK();
}

} // namespace primihub::service
