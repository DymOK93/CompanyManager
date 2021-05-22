#pragma once
/*Engine headers*/
#include "company_manager_engine.h"
#include "entry_view.h"
#include "file_io_command.h"
#include "internal_workers.h"
#include "message.h"
#include "new_entry_dialog.h"
#include "task_manager.h"
#include "tree_model.h"
#include "tree_model_command.h"
#include "xml_wrapper_command.h"

/*GUI forms*/
#include "ui_company_manager_ui.h"

/*Standart Library headers*/
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <unordered_map>

/*Qt headers*/
#include <QEvent>
#include <QFile>
#include <QString>
#include <QtWidgets>

class CompanyManagerUI : public QMainWindow {
  Q_OBJECT
 public:
  using Company = wrapper::Company;
  using Department = wrapper::Department;
  using Employee = wrapper::Employee;
  using PipelineBuilder =
      worker::ui_internal::PipelineBuilder<bool>;  // bool-флаг для контроля
                                                   // успешного выполнения
                                                   // разнотипных операций
  using DepartmentViewInfo = DepartmentView::view_info;
  using EmployeeViewInfo = EmployeeView::view_info;
  using EmployeePersonalFile = command::xml_wrapper::EmployeePersonalFile;

 public:
  CompanyManagerUI(QWidget* parent = nullptr);
 public slots:

  /*Меню "Файл"*/
  bool create();
  bool load();
  bool save();
  bool save_as();
  void close();
  void exit();

  /*Меню "Правка"*/
  bool undo();
  bool redo();

  /*Меню "О программе"*/
  void about_program() const;

 private slots:
  void selection_changed(const QItemSelection& selected,
                         const QItemSelection& deselected);
  bool new_department();
  bool remove_department(const DepartmentViewInfo& view_info);
  bool rename_department(const DepartmentViewInfo& view_info,
                         const QString& new_name);
  bool new_employee(const DepartmentViewInfo& view_info);
  bool remove_employee(const EmployeeViewInfo& view_info);
  bool change_employee_surname(const EmployeeViewInfo& view_info,
                               const QString& new_middle_name);
  bool change_employee_name(const EmployeeViewInfo& view_info,
                            const QString& new_middle_name);
  bool change_employee_middle_name(const EmployeeViewInfo& view_info,
                                   const QString& new_middle_name);
  bool change_employee_function(const EmployeeViewInfo& view_info,
                                const QString& new_function);
  bool update_employee_salary(const EmployeeViewInfo& view_info,
                              Employee::salary_t new_salary);

 private:
  /*Вспомогательные структуры*/

  struct TaskManagement {
    TaskManager<CompanyManager> service{TaskManager<CompanyManager>(
        0)};  //Команды не добавляются в очередля для отмены
    TaskManager<CompanyManager>
        modify_xml;  //Стандартная емкость очереди команд (20)
    TaskManager<CompanyTreeModel> tree_model;
  };
  struct FileHandlers {
    mutable QFileInfo info;
    QFileDialog dialog;
  };
  struct ItemViewers {  //Объектами будет владеть QStackedWidget
    QWidget* dummy;  //Заглушка, если выделение отсутствует
    DepartmentView* department;
    EmployeeView* employee;
  };

 private:
  /*Инициализация интерфейса главного окна*/
  void connect_menu_bar();
  void initialize_stacked_viewers();
  void connect_item_viewers();

  /*Отмена и повтор операций*/
  void reset_redo_queues();  //Сброс возможности повтора действия
  void update_undo_redo_buttons();  //Активация/деактивация кнопок "Отменить" и
                                    //"Вернуть"
  void update_redo_undo_actions_after_editing();  //Объединяет два
                                                  //вышеперечисленных действия
  template <
      class FirstHandler,
      class SecondHandler>  //Для смены последовательности повтора операций
  bool redo_helper(FirstHandler&& first_handler,
                   SecondHandler&& second_handler) {
    bool success{false};
    PipelineBuilder()
        .AddHandler(std::forward<FirstHandler>(first_handler))
        .AddHandler(std::forward<SecondHandler>(second_handler))
        .Assemble()
        ->Process(success);
    return success;
  }

  /*Настройка и сброс элементов отображения*/
  void update_viewers();  //Ручное обновление после отмены или повтора операции
  void setup_viewers();  //Настройка после открытия нового файла
  void select_viewer(const QModelIndex& selected_item);
  void update_department_viewer(const QModelIndex& selected_item);
  void update_employee_viewer(const QModelIndex& selected_item);
  void show_viewers(bool on);
  QModelIndex get_current_selection_index();

  /*Сброс и действия при закрытии*/
  enum class WarningMode {  //Режим показа предупреждающих сообщений
    Hide,
    Show
  };
  void reset_helper();
  bool close_helper(WarningMode no_loaded);
  void closeEvent(QCloseEvent* event)
      override;  //Перехват события закрытия для запроса соранения файла

 private:
  /*Обработка и логирование ошибок, предупреждающие сообщения*/

  QMessageBox::StandardButton show_message(
      message::Type,
      QString title,
      std::optional<QString> text = std::nullopt,
      QMessageBox::StandardButtons buttons =
          QMessageBox::Button::NoButton) const;

  template <class TargetTy>
  void handle_internal_fatal_error(TaskManager<TargetTy>& tm) const noexcept {
    internal_fatal_error_msg(u8"Критическая ошибка", tm.GetErrorLog().back());
  }
  void internal_fatal_error_msg(
      QString title,
      const std::string& error_definition = "") const noexcept;

  QMessageBox::StandardButton
  save_before_close_request(  //В запросе сохранения файла действие зависит от
                              //нажатой пользователем кнопки
      const QString& file_name);

  void empty_field_msg(const QString& field) const;
  void already_exists_msg(const QString& element_category)
      const;  // return type: int => запрашивает пользовательское действие
  int empty_optional_field_msg_dlg(const QString& field) const;
  int zero_salary_msg_dlg() const;
  void not_loaded_msg() const;
  void invalid_load_path_msg() const;
  void invalid_save_path_msg() const;
  void unable_to_load_msg() const;
  void unable_to_save_msg() const;
  void already_saved_msg();
  bool warning_if_employee_already_exists(wrapper::RenameResult result)
      const;  //Предупреждение и возврат false, если result != Success
  bool warning_if_department_already_exists(wrapper::RenameResult result) const;

  /*Запрос и обработка имени файла*/
  QString request_load_file_path();
  QString request_save_file_path();
  QString get_current_file_path();
  QString extract_file_name(
      const QString& path);  //Возвращает имя файла без расширения

  bool is_loaded() const;
  bool is_saved() const;
  void set_file_path(const QString& path);

  /*Проверка возможности чтения и записи файлов*/
  bool is_readable(const QString& path) const;
  bool is_writable(const QString& path) const;

  /*Вспомогательные методы для инициализации при создании, загрузке из файла и
   * закрытии документа*/
  bool create_helper();
  std::optional<worker::file_operation::Result> load_helper();
  bool handle_load_result(worker::file_operation::Result result);
  std::optional<worker::file_operation::Result> save_helper();
  bool handle_save_result(worker::file_operation::Result result);
  std::optional<Company::department_view_it> add_department_helper(
      Department&& department);  //Результат и итератор
  std::optional<Company::department_view_it> insert_department_helper(
      Department&& department,
      wrapper::string_ref before);  //Экономим 1 вызов move c-tor
  bool rename_department_helper(const DepartmentViewInfo& view_info,
                                const QString& new_name);
  bool rename_employee_in_tree_model(
      const EmployeeViewInfo& view_info,
      const QString& new_full_name);  //Однотипные изменения модели стоит
                                      //вынести в отдельный метод

  /*Распаковка данных, переданных элементами интерфейса */
  static const Department* extract_department_ptr(
      const DepartmentViewInfo& view_info);
  static wrapper::string_ref extract_department_name(
      const DepartmentViewInfo& view_info);
  static wrapper::string_ref extract_department_name(
      const EmployeeViewInfo& view_info);
  static EmployeePersonalFile extract_employee_personal_data(
      const EmployeeViewInfo& view_info);

 private:
  /*Фабрики сообщений*/
  message::Plantation m_message_factories{
      message::MakeFactory<message::Question>(),
      message::MakeFactory<message::Info>(),
      message::MakeFactory<message::Warning>(),
      message::MakeFactory<message::Critical>()};

  /*GUI*/
  std::unique_ptr<Ui::CompanyManagerGUI> m_gui{
      std::make_unique<Ui::CompanyManagerGUI>()};
  std::unique_ptr<CompanyTreeModel> m_tree_model{
      std::make_unique<CompanyTreeModel>()};
  ItemViewers m_item_viewers;

  /*Основной движок*/
  std::unique_ptr<CompanyManager> m_company_manager{
      std::make_unique<CompanyManager>()};
  std::unique_ptr<TaskManagement> m_tasks{std::make_unique<TaskManagement>()};
  std::unique_ptr<FileHandlers> m_file_handlers{
      std::make_unique<FileHandlers>()};
};
