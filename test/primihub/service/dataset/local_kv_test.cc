

#include "gtest/gtest.h"
#include "src/primihub/service/dataset/localkv/storage_leveldb.h"
#include "src/primihub/service/error.hpp" 

namespace primihub::service {
TEST(LocalKVTest, PutGetErase) {
    StorageBackendLevelDB storage;
    Key key("testkey");
    Value value = "testvalue";
    ASSERT_EQ(storage.putValue(key, value), outcome::success());
    ASSERT_EQ(storage.getValue(key), outcome::success(value));
    ASSERT_EQ(storage.erase(key), outcome::success());
    auto e = storage.getValue(key);
    ASSERT_EQ(e.error(), Error::VALUE_NOT_FOUND);
}

} // namespace primihub::service
