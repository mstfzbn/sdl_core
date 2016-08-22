/*
 * Copyright (c) 2016, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "mobile/register_app_interface_request.h"
#include "utils/shared_ptr.h"
#include "smart_objects/smart_object.h"
#include "commands/commands_test.h"
#include "commands/command_request_test.h"
#include "application_manager/application.h"
#include "application_manager/mock_application_manager.h"
#include "application_manager/mock_application.h"
#include "interfaces/MOBILE_API.h"
#include "application_manager/smart_object_keys.h"
#include "application_manager/policies/mock_policy_handler_interface.h"
#include "utils/data_accessor.h"
#include "mock_message_helper.h"
#include "protocol_handler/mock_session_observer.h"
#include "connection_handler/mock_connection_handler.h"
#include "mock_hmi_capabilities.h"
#include "mock_resume_ctrl.h"
#include "utils/custom_string.h"
#include "utils/lock.h"

namespace test {
namespace components {
namespace commands_test {
namespace mobile_commands_test {

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::DoAll;

namespace am = ::application_manager;

using ::utils::SharedPtr;
using am::commands::MessageSharedPtr;
using am::commands::RegisterAppInterfaceRequest;

namespace {
const uint32_t kConnectionKey = 1u;
const hmi_apis::Common_Language::eType kHmiLanguage =
    hmi_apis::Common_Language::EN_US;
const mobile_apis::Language::eType kMobileLanguage =
    mobile_apis::Language::EN_US;
const std::string kMacAddress = "test_mac_address";
const std::string kAppId = "test_app_id";
const std::string kDummyString = "test_string";
const std::vector<uint32_t> kDummyDiagModes;
}  // namespace

class RegisterAppInterfaceRequestTest
    : public CommandRequestTest<CommandsTestMocks::kIsNice> {
 public:
  RegisterAppInterfaceRequestTest()
      : msg_(CreateMessage())
      , command_(CreateCommand<RegisterAppInterfaceRequest>(msg_))
      , app_name_("test_app_name_")
      , mock_message_helper_(*am::MockMessageHelper::message_helper_mock()) {}

  void InitBasicMessage() {
    (*msg_)[am::strings::params][am::strings::connection_key] = kConnectionKey;
    (*msg_)[am::strings::msg_params][am::strings::app_id] = kAppId;
    (*msg_)[am::strings::msg_params][am::strings::app_name] = app_name_;
    (*msg_)[am::strings::msg_params][am::strings::language_desired] =
        kHmiLanguage;
    (*msg_)[am::strings::msg_params]
           [am::strings::hmi_display_language_desired] = kHmiLanguage;
  }

  MockAppPtr CreateBasicMockedApp() {
    MockAppPtr mock_app = CreateMockApp();
    ON_CALL(*mock_app, name()).WillByDefault(ReturnRef(app_name_));
    ON_CALL(*mock_app, mac_address()).WillByDefault(ReturnRef(kMacAddress));
    ON_CALL(*mock_app, app_icon_path()).WillByDefault(ReturnRef(kDummyString));
    ON_CALL(*mock_app, language()).WillByDefault(ReturnRef(kMobileLanguage));
    ON_CALL(*mock_app, ui_language()).WillByDefault(ReturnRef(kMobileLanguage));
    return mock_app;
  }

  void InitLanguage(
      hmi_apis::Common_Language::eType ui_language = kHmiLanguage,
      hmi_apis::Common_Language::eType vr_language = kHmiLanguage) {
    ON_CALL(mock_hmi_capabilities_, active_vr_language())
        .WillByDefault(Return(vr_language));
    ON_CALL(mock_hmi_capabilities_, active_ui_language())
        .WillByDefault(Return(ui_language));
  }

  void InitGetters() {
    ON_CALL(app_mngr_, GetPolicyHandler())
        .WillByDefault(ReturnRef(mock_policy_handler_));
    ON_CALL(app_mngr_, resume_controller())
        .WillByDefault(ReturnRef(mock_resume_crt_));
    ON_CALL(app_mngr_, connection_handler())
        .WillByDefault(ReturnRef(mock_connection_handler_));
    ON_CALL(mock_connection_handler_, get_session_observer())
        .WillByDefault(ReturnRef(mock_session_observer_));
    ON_CALL(app_mngr_, hmi_capabilities())
        .WillByDefault(ReturnRef(mock_hmi_capabilities_));
    ON_CALL(app_mngr_settings_, sdl_version())
        .WillByDefault(ReturnRef(kDummyString));
    ON_CALL(mock_hmi_capabilities_, ccpu_version())
        .WillByDefault(ReturnRef(kDummyString));
    ON_CALL(app_mngr_settings_, supported_diag_modes())
        .WillByDefault(ReturnRef(kDummyDiagModes));
    ON_CALL(mock_policy_handler_, GetAppRequestTypes(_))
        .WillByDefault(Return(std::vector<std::string>()));
    ON_CALL(mock_policy_handler_, GetUserConsentForDevice(_))
        .WillByDefault(Return(policy::DeviceConsent::kDeviceAllowed));
    ON_CALL(app_mngr_, GetDeviceTransportType(_))
        .WillByDefault(Return(hmi_apis::Common_TransportType::WIFI));
  }

  MessageSharedPtr msg_;
  SharedPtr<RegisterAppInterfaceRequest> command_;

  const utils::custom_string::CustomString app_name_;
  sync_primitives::Lock lock_;
  am::ApplicationSet app_set_;

  typedef IsNiceMock<policy_test::MockPolicyHandlerInterface,
                     kMocksAreNice>::Result MockPolicyHandlerInterface;

  typedef IsNiceMock<resumption::MockResumeCtrl, kMocksAreNice>::Result
      MockResumeCtrl;

  typedef IsNiceMock<connection_handler_test::MockConnectionHandler,
                     kMocksAreNice>::Result MockConnectionHandler;

  typedef IsNiceMock<protocol_handler_test::MockSessionObserver,
                     kMocksAreNice>::Result MockSessionObserver;

  typedef IsNiceMock<application_manager_test::MockHMICapabilities,
                     kMocksAreNice>::Result MockHMICapabilities;

  MockPolicyHandlerInterface mock_policy_handler_;
  MockResumeCtrl mock_resume_crt_;
  MockConnectionHandler mock_connection_handler_;
  MockSessionObserver mock_session_observer_;
  MockHMICapabilities mock_hmi_capabilities_;

  am::MockMessageHelper& mock_message_helper_;
};

TEST_F(RegisterAppInterfaceRequestTest, Run_MinimalData_SUCCESS) {
  InitGetters();
  InitLanguage();
  InitBasicMessage();

  ON_CALL(app_mngr_, IsHMICooperating()).WillByDefault(Return(true));

  MockAppPtr mock_app = CreateBasicMockedApp();
  EXPECT_CALL(app_mngr_, application(kConnectionKey))
      .WillOnce(Return(ApplicationSharedPtr()))
      .WillRepeatedly(Return(mock_app));

  ON_CALL(app_mngr_, applications())
      .WillByDefault(Return(DataAccessor<am::ApplicationSet>(app_set_, lock_)));
  ON_CALL(mock_policy_handler_, PolicyEnabled()).WillByDefault(Return(true));
  ON_CALL(mock_policy_handler_, GetInitialAppData(kAppId, _, _))
      .WillByDefault(Return(true));

  EXPECT_CALL(app_mngr_, RegisterApplication(msg_)).WillOnce(Return(mock_app));
  EXPECT_CALL(app_mngr_,
              ManageHMICommand(HMIResultCodeIs(
                  hmi_apis::FunctionID::BasicCommunication_OnAppRegistered)))
      .WillOnce(Return(true));
  EXPECT_CALL(app_mngr_,
              ManageHMICommand(HMIResultCodeIs(
                  hmi_apis::FunctionID::Buttons_OnButtonSubscription)))
      .WillOnce(Return(true));
  EXPECT_CALL(
      app_mngr_,
      ManageMobileCommand(MobileResultCodeIs(mobile_apis::Result::SUCCESS), _));
  EXPECT_CALL(mock_message_helper_,
              SendChangeRegistrationRequestToHMI(testing::Eq(mock_app), _));
  command_->Run();
}

ACTION_P(SetHmiType, hmi_type) {
  arg2->push_back(hmi_type);
}

// TODO(SLevchenko): Enable test after fixes from PR #702 and #764 will be
// merged in current branch.
TEST_F(RegisterAppInterfaceRequestTest, DISABLED_Run_AppHmiTypes_SUCCESS) {
  const mobile_apis::AppHMIType::eType app_hmi_type =
      mobile_apis::AppHMIType::DEFAULT;
  const std::string app_hmi_type_str("DEFAULT");
  InitGetters();
  InitLanguage();
  InitBasicMessage();

  (*msg_)[am::strings::msg_params][am::strings::app_hmi_type][0] = app_hmi_type;

  ON_CALL(app_mngr_, IsHMICooperating()).WillByDefault(Return(true));

  MockAppPtr mock_app = CreateBasicMockedApp();
  EXPECT_CALL(app_mngr_, application(kConnectionKey))
      .WillOnce(Return(ApplicationSharedPtr()))
      .WillRepeatedly(Return(mock_app));

  ON_CALL(app_mngr_, applications())
      .WillByDefault(Return(DataAccessor<am::ApplicationSet>(app_set_, lock_)));
  ON_CALL(mock_policy_handler_, PolicyEnabled()).WillByDefault(Return(true));
  ON_CALL(mock_policy_handler_, GetInitialAppData(kAppId, _, _))
      .WillByDefault(DoAll(SetHmiType(app_hmi_type_str), Return(true)));

  EXPECT_CALL(app_mngr_, RegisterApplication(msg_)).WillOnce(Return(mock_app));
  EXPECT_CALL(app_mngr_,
              ManageHMICommand(HMIResultCodeIs(
                  hmi_apis::FunctionID::BasicCommunication_OnAppRegistered)))
      .WillOnce(Return(true));
  EXPECT_CALL(app_mngr_,
              ManageHMICommand(HMIResultCodeIs(
                  hmi_apis::FunctionID::Buttons_OnButtonSubscription)))
      .WillOnce(Return(true));
  EXPECT_CALL(
      app_mngr_,
      ManageMobileCommand(MobileResultCodeIs(mobile_apis::Result::SUCCESS), _));
  EXPECT_CALL(mock_message_helper_,
              SendChangeRegistrationRequestToHMI(testing::Eq(mock_app), _));
  command_->Run();
}

}  // namespace mobile_commands_test
}  // namespace commands_test
}  // namespace components
}  // namespace test
