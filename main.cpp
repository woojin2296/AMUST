#include <QApplication>
#include <QMainWindow>
#include <QStackedWidget>

#include "boot_screen_widget.h"
#include "main_menu_widget.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  QMainWindow window;
  window.setWindowTitle("AMUST v0.2.0");
  window.setFixedSize(1024, 600);

  auto *stack = new QStackedWidget(&window);
  auto *boot = new BootScreenWidget(stack);
  auto *menu = new MainMenuWidget(stack);

  stack->addWidget(boot);
  stack->addWidget(menu);
  stack->setCurrentWidget(boot);
  window.setCentralWidget(stack);

  QObject::connect(boot, &BootScreenWidget::continueRequested, stack,
                   [stack, menu]() { stack->setCurrentWidget(menu); });

  window.show();
  return app.exec();
}
