#include "entry_view.h"

DepartmentView::DepartmentView(QWidget* parent)
	: MyBase(parent) {
	m_gui->setupUi(this);
	connect(m_gui->enable_editing_chbox, &QCheckBox::stateChanged, this, &DepartmentView::view_mode);
	connect(m_gui->name_lnedit, &QLineEdit::returnPressed, this, &DepartmentView::name_changed_transmitter);
	connect(m_gui->erase_department_btn, &QPushButton::clicked, this, &DepartmentView::erased_transmitter);
	connect(m_gui->add_employee_btn, &QPushButton::clicked, this, &DepartmentView::employee_added_transmitter);
	m_gui->employee_count_lnedit->setReadOnly(true);			//—татистические сведени€ не могут быть отредактированы вручную
	m_gui->average_salary_lnedit->setReadOnly(true);
}

void DepartmentView::erased_transmitter() {
	emit erased(MyBase::get_view());							
}

void DepartmentView::UpdateView() {
	RestoreName();
	UpdateStats();
}


void DepartmentView::RestoreName() {
	m_gui->name_lnedit->setText(
		QString::fromStdString(MyBase::get_item<const wrapper::Department*>()->GetName())
	);
}

void DepartmentView::UpdateStats() {
	auto& department_it{ MyBase::get_item<const wrapper::Department*>() };				//«апрос статистики подразделени€
	m_gui->employee_count_lnedit->setText(
		QString::number(department_it->EmployeeCount())
	);
	m_gui->average_salary_lnedit->setText(
		QString::number(department_it->AverageSalary())
	);
}

void DepartmentView::view_mode(int mode) {
	switch (mode) {
	case Qt::CheckState::Unchecked: set_read_only_mode(true); break;
	case Qt::CheckState::Checked: set_read_only_mode(false); break;
	};
}

void DepartmentView::name_changed_transmitter() {
	emit name_changed(MyBase::get_view(), m_gui->name_lnedit->text());
}

void DepartmentView::employee_added_transmitter() {
	emit employee_added(MyBase::get_view());
}

DepartmentView& DepartmentView::SetDepartment(const view_info& department) {
	MyBase::SetViewInfo(department);
	UpdateView();
	set_read_only_mode(true);
	uncheck_enable_editing_chbox();							//—брос состо€ни€ кнопки
	return*this;
}

DepartmentView& DepartmentView::Reset() noexcept {
	MyBase::Reset();
	uncheck_enable_editing_chbox();
	return* this;
}

void DepartmentView::set_read_only_mode(bool on) {
	m_gui->name_lnedit->setReadOnly(on);									//«апрет или разрешение редактировани€ наименовани€
}

void DepartmentView::uncheck_enable_editing_chbox() {
	m_gui->enable_editing_chbox->setChecked(false);
}

EmployeeView::EmployeeView(QWidget* parent)
	: MyBase(parent) {

	m_gui->setupUi(this);
	m_gui->salary_lnedit->setValidator(m_int_validator.get());				//ѕроверка, что значение €вл€етс€ целым числом

	connect(m_gui->enable_editing_chbox, &QCheckBox::stateChanged, this, &EmployeeView::view_mode);
	connect(m_gui->surname_lnedit, &QLineEdit::returnPressed, this, &EmployeeView::surname_changed_transmitter);
	connect(m_gui->name_lnedit, &QLineEdit::returnPressed, this, &EmployeeView::name_changed_transmitter);
	connect(m_gui->middle_name_lnedit, &QLineEdit::returnPressed, this, &EmployeeView::middle_name_changed_transmitter);
	connect(m_gui->function_lnedit, &QLineEdit::returnPressed, this, &EmployeeView::function_changed_transmitter);
	connect(m_gui->salary_lnedit, &QLineEdit::returnPressed, this, &EmployeeView::salary_updated_transmitter);
	connect(m_gui->erase_employee_btn, &QPushButton::pressed, this, &EmployeeView::erased_transmitter);

}

void EmployeeView::view_mode(int mode) {
	switch (mode) {
	case Qt::CheckState::Unchecked: set_read_only_mode(true); break;
	case Qt::CheckState::Checked: set_read_only_mode(false); break;
	};
}

void EmployeeView::set_read_only_mode(bool on) {
	m_gui->surname_lnedit->setReadOnly(on);
	m_gui->name_lnedit->setReadOnly(on);
	m_gui->middle_name_lnedit->setReadOnly(on);
	m_gui->function_lnedit->setReadOnly(on);
	m_gui->salary_lnedit->setReadOnly(on);
}

void EmployeeView::uncheck_enable_editing_chbox() {
	m_gui->enable_editing_chbox->setChecked(false);
}

EmployeeView& EmployeeView::SetEmployee(const view_info& employee) {
	MyBase::SetViewInfo(employee);
	UpdateView();	
	set_read_only_mode(true);
	uncheck_enable_editing_chbox();
	return *this;
}
EmployeeView& EmployeeView::Reset() noexcept {
	MyBase::Reset();
	uncheck_enable_editing_chbox();
	return *this;
}

void EmployeeView::UpdateModelIndex(const QModelIndex& new_q_idx) {
	MyBase::update_model_index(new_q_idx);
}

void EmployeeView::UpdateView() {
	RestoreSurname();
	RestoreName();
	RestoreMiddleName();
	RestoreFunction();
	RestoreSalary();
}

void EmployeeView::RestoreSurname() {
	m_gui->surname_lnedit->setText(
		QString::fromStdString(MyBase::get_item<const wrapper::Employee*>()->GetSurname().get().data())
	);
}

void EmployeeView::RestoreName() {
	m_gui->name_lnedit->setText(
		QString::fromStdString(MyBase::get_item<const wrapper::Employee*>()->GetName().get().data())
	);
}

void EmployeeView::RestoreMiddleName() {
	m_gui->middle_name_lnedit->setText(
		QString::fromStdString(MyBase::get_item<const wrapper::Employee*>()->GetMiddleName().get().data())
	);
}

void EmployeeView::RestoreFunction() {
	m_gui->function_lnedit->setText(
		QString::fromStdString(MyBase::get_item<const wrapper::Employee*>()->GetFunction().get().data())
	);
}

void EmployeeView::RestoreSalary() {
	m_gui->salary_lnedit->setText(
		QString::number(MyBase::get_item<const wrapper::Employee*>()->GetSalary())
	);
}

void EmployeeView::surname_changed_transmitter() {
	emit surname_changed(
		MyBase::get_view(),
		m_gui->surname_lnedit->text()
	);
}

void EmployeeView::name_changed_transmitter() {
	emit name_changed(
		MyBase::get_view(),
		m_gui->name_lnedit->text()
	);
}

void EmployeeView::middle_name_changed_transmitter() {
	emit middle_name_changed(
		MyBase::get_view(),
		m_gui->middle_name_lnedit->text()
	);
}

void EmployeeView::function_changed_transmitter() {
	emit function_changed(
		MyBase::get_view(),
		m_gui->function_lnedit->text()
	);
}

void EmployeeView::salary_updated_transmitter() {
	emit salary_updated(
		MyBase::get_view(),
		static_cast<wrapper::Employee::salary_t>(m_gui->salary_lnedit->text().toInt())
	);
}

void EmployeeView::erased_transmitter() {
	emit erased(MyBase::get_view());
}




