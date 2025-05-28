#include <Home.h>

#include <QApplication>

#include "contest.h"
#include "custom_controls/avatarcrop.h"
#include "custom_controls/dialog.h"
#include "custom_controls/facedetection.h"
#include "login.h"
#include "network_modules/apiclient.h"
#include "register.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

#if 1
    Login w;
    QPointer<Home> home = nullptr;
    Home::connect(&w, &Login::sigloginSucceed, [&](const QJsonObject& userInfo)
                  {
                      if (!home)
                      {
                          home = new Home(userInfo);
                          home->show();
                      } });
#else
    // QPointer<ApiClient> client = new ApiClient("100000005");
    // FaceDetection w(client);
    // Contest w({}, nullptr);
    // Home w("");
    Dialog msgBox(nullptr);
    msgBox.setNotice();
    msgBox.setText("验证失败:\n人脸认证失败");
    msgBox.show();
#endif

    w.show();

    return a.exec();
}
