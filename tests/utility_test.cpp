//
// Created by whalien on 28/03/23.
//
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/sa_utility.h"
#include "mergebot/filesystem.h"

TEST(UtilityTest, ExtractOneConflictFile) {
  std::vector<mergebot::sa::ConflictBlock> ExpectedConflictBlocks = {
      {.Index = 1,
       .ConflictRange =
           "<<<<<<< "
           "/home/whalien/Desktop/projects/frameworks_av/conflicts/"
           "2d74c3f32d3035750b1e31fec80e5e3d5e6e1061/services/oboeservice/"
           "AAudioServiceEndpoint.cpp/conflict.cpp\n"
           "std::vector<android::sp<AAudioServiceStreamBase>>\n"
           "        AAudioServiceEndpoint::disconnectRegisteredStreams() {\n"
           "    std::vector<android::sp<AAudioServiceStreamBase>> "
           "streamsDisconnected;\n"
           "    {\n"
           "        std::lock_guard<std::mutex> lock(mLockStreams);\n"
           "        mRegisteredStreams.swap(streamsDisconnected);\n"
           "    }\n"
           "||||||| "
           "/home/whalien/Desktop/projects/frameworks_av/conflicts/"
           "2d74c3f32d3035750b1e31fec80e5e3d5e6e1061/services/oboeservice/"
           "AAudioServiceEndpoint.cpp/base.cpp\n"
           "void AAudioServiceEndpoint::disconnectRegisteredStreams() {\n"
           "    std::lock_guard<std::mutex> lock(mLockStreams);\n"
           "=======\n"
           "void AAudioServiceEndpoint::disconnectRegisteredStreams() {\n"
           "    std::vector<android::sp<AAudioServiceStreamBase>> "
           "streamsToDisconnect;\n"
           "    {\n"
           "        std::lock_guard<std::mutex> lock(mLockStreams);\n"
           "        mRegisteredStreams.swap(streamsToDisconnect);\n"
           "    }\n"
           "    // Stop and disconnect outside mLockStreams to avoid reverse\n"
           "    // ordering of AAudioServiceStreamBase::mLock and "
           "mLockStreams\n"
           ">>>>>>> "
           "/home/whalien/Desktop/projects/frameworks_av/conflicts/"
           "2d74c3f32d3035750b1e31fec80e5e3d5e6e1061/services/oboeservice/"
           "AAudioServiceEndpoint.cpp/theirs.cpp\n"},
      {.Index = 2,
       .ConflictRange =
           "<<<<<<< "
           "/home/whalien/Desktop/projects/frameworks_av/conflicts/"
           "2d74c3f32d3035750b1e31fec80e5e3d5e6e1061/services/oboeservice/"
           "AAudioServiceEndpoint.cpp/conflict.cpp\n"
           "    // We need to stop all the streams before we disconnect them.\n"
           "    // Otherwise there is a race condition where the first "
           "disconnected app\n"
           "    // tries to reopen a stream as MMAP but is blocked by the "
           "second stream,\n"
           "    // which hasn't stopped yet. Then the first app ends up with a "
           "Legacy stream.\n"
           "    for (const auto &stream : streamsDisconnected) {\n"
           "        ALOGD(\"%s() - stop(), port = %d\", __func__, "
           "stream->getPortHandle());\n"
           "||||||| "
           "/home/whalien/Desktop/projects/frameworks_av/conflicts/"
           "2d74c3f32d3035750b1e31fec80e5e3d5e6e1061/services/oboeservice/"
           "AAudioServiceEndpoint.cpp/base.cpp\n"
           "    for (const auto& stream : mRegisteredStreams) {\n"
           "        ALOGD(\"disconnectRegisteredStreams() stop and disconnect "
           "port %d\",\n"
           "              stream->getPortHandle());\n"
           "=======\n"
           "    for (const auto& stream : streamsToDisconnect) {\n"
           "        ALOGD(\"disconnectRegisteredStreams() stop and disconnect "
           "port %d\",\n"
           "              stream->getPortHandle());\n"
           ">>>>>>> "
           "/home/whalien/Desktop/projects/frameworks_av/conflicts/"
           "2d74c3f32d3035750b1e31fec80e5e3d5e6e1061/services/oboeservice/"
           "AAudioServiceEndpoint.cpp/theirs.cpp\n"},
      {.Index = 3,
       .ConflictRange =
           "<<<<<<< "
           "/home/whalien/Desktop/projects/frameworks_av/conflicts/"
           "2d74c3f32d3035750b1e31fec80e5e3d5e6e1061/services/oboeservice/"
           "AAudioServiceEndpoint.cpp/conflict.cpp\n"
           "    return streamsDisconnected;\n"
           "}\n"
           "\n"
           "void AAudioServiceEndpoint::releaseRegisteredStreams() {\n"
           "    // List of streams to be closed after we disconnect "
           "everything.\n"
           "    std::vector<android::sp<AAudioServiceStreamBase>> "
           "streamsToClose\n"
           "            = disconnectRegisteredStreams();\n"
           "\n"
           "    // Close outside the lock to avoid recursive locks.\n"
           "    AAudioService *aaudioService = "
           "AAudioClientTracker::getInstance().getAAudioService();\n"
           "    for (const auto& serviceStream : streamsToClose) {\n"
           "        ALOGD(\"%s() - close stream 0x%08X\", __func__, "
           "serviceStream->getHandle());\n"
           "        aaudioService->closeStream(serviceStream);\n"
           "    }\n"
           "||||||| "
           "/home/whalien/Desktop/projects/frameworks_av/conflicts/"
           "2d74c3f32d3035750b1e31fec80e5e3d5e6e1061/services/oboeservice/"
           "AAudioServiceEndpoint.cpp/base.cpp\n"
           "    mRegisteredStreams.clear();\n"
           "=======\n"
           ">>>>>>> "
           "/home/whalien/Desktop/projects/frameworks_av/conflicts/"
           "2d74c3f32d3035750b1e31fec80e5e3d5e6e1061/services/oboeservice/"
           "AAudioServiceEndpoint.cpp/theirs.cpp\n"}};

  std::string MockFilePath =
      "/home/whalien/codebase/cpp/mergebot/build/bin/mock/conflict.cpp";
  mergebot::sa::ConflictFile ExpectedConflictFile(MockFilePath,
                                                  ExpectedConflictBlocks);
  std::vector<mergebot::sa::ConflictFile> ExpectedConflictFiles = {
      ExpectedConflictFile};
  std::vector<std::string> ConflictFilePaths = {
      (std::filesystem::current_path() / "mock" / "conflict.cpp").string()};
  std::vector<mergebot::sa::ConflictFile> ConflictFiles =
      mergebot::sa::constructConflictFiles(ConflictFilePaths);

  ASSERT_EQ(ExpectedConflictFiles, ConflictFiles);
}
