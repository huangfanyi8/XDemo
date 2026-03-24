#include <QApplication>
#include <QMainWindow>
#include <QSplitter>
#include <QListWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QDrag>
#include <QMimeData>
#include <QDebug>
#include <QPainter>
#include <QSplitter>
#include <QPropertyAnimation>
#include <cmath>
#include <QInputDialog>
#include<QMenu>
#include <QPushButton>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <cctype>
#include <stdexcept>
#include<QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QShowEvent>
#include<QAction>
namespace NodeEditor
{
    // ---------------------- AST ----------------------
    struct AstNodeBase { virtual ~AstNodeBase() = default; };

    struct AstCompare : AstNodeBase
    {
    std::string lhs;
    std::string op;
    int rhs;
    AstCompare(const std::string& l, const std::string& o, int r)
        : lhs(l), op(o), rhs(r) {}
    };

    struct AstLogical : AstNodeBase
    {
    std::string op;
    std::unique_ptr<AstNodeBase> left;
    std::unique_ptr<AstNodeBase> right;
    AstLogical(const std::string& o, std::unique_ptr<AstNodeBase> l, std::unique_ptr<AstNodeBase> r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    };

    struct CompareNode
    {
    int id;
    std::string lhs;
    std::string op;
    int rhs;
    };

    struct LogicNode
    {
    int id;
    std::string op;
    std::vector<int> inputs;
    };

    class Graph
    {
    public:
    int add_compare_node(const std::string& lhs, const std::string& op, int rhs)
    {
    int id = next_id_++;
    compares_.push_back({id, lhs, op, rhs});
    return id;
    }

    int add_logic_node(const std::string& op, std::vector<int> inputs)
    {
    int id = next_id_++;
    logics_.push_back({id, op, inputs});
    return id;
    }

    [[nodiscard]] bool is_compare_node(int id) const
    {
    for (auto& n : compares_) if (n.id == id) return true;
    return false;
    }

    [[nodiscard]] bool is_logic_node(int id) const
    {
    for (auto& n : logics_) if (n.id == id) return true;
    return false;
    }

    [[nodiscard]] const CompareNode* get_compare_node(int id) const
    {
    for (auto& n : compares_) if (n.id == id) return &n;
    return nullptr;
    }

    [[nodiscard]] const LogicNode* get_logic_node(int id) const
    {
    for (auto& n : logics_) if (n.id == id) return &n;
    return nullptr;
    }

    private:
    int next_id_ = 1;
    //应该用variant优化
    std::vector<CompareNode> compares_;
    std::vector<LogicNode> logics_;
    };
    // ---------------------- AST Builder ----------------------
    class AstBuilder
    {
    public:
        explicit AstBuilder(const Graph& graph) : graph_(graph) {}
        std::unique_ptr<AstNodeBase> build(int root_id)
        {
            return build_node_(root_id);
        }

    private:
        std::unique_ptr<AstNodeBase> build_node_(int id)
        {
            if (graph_.is_compare_node(id))
            {
                const auto* c = graph_.get_compare_node(id);
                return std::make_unique<AstCompare>(c->lhs, c->op, c->rhs);
            }
            else if (graph_.is_logic_node(id))
            {
                const auto* l = graph_.get_logic_node(id);
                return std::make_unique<AstLogical>(
                    l->op,
                    build_node_(l->inputs[0]),
                    build_node_(l->inputs[1])
                );
            }
            else
                throw std::runtime_error("unknown node id");
        }

        const Graph& graph_;
    };

    // ---------------------- CppEmitter ----------------------
    class CppEmitter
    {
    public:
        // 生成表达式字符串
        std::string emit_expr(const AstNodeBase* node)
        {
            std::ostringstream oss;
            emit_node_(node, oss);
            return oss.str();
        }

        // 生成完整 C++ 文件源码字符串
        std::string emit_cpp_code(const AstNodeBase* node)
        {
            std::unordered_map<std::string,std::string> vars;
            this->collect_variables(node, vars);
            std::ostringstream oss;
            oss << "#include <iostream>\n\nint main()\n{\n"
                << generate_var_decls(vars)
                << "    bool result = " << emit_expr(node) << ";\n"
                << "    std::cout << \"Result: \" << std::boolalpha << result << std::endl;\n"
                << "    return 0;\n}\n";
            return oss.str();
        }

    private:
        void emit_node_(const AstNodeBase* node, std::ostringstream& oss)
        {
            if (auto* c = dynamic_cast<const AstCompare*>(node))
            {
                oss  << c->lhs << " " << c->op << " " << c->rhs;
            }
            else if (auto* l = dynamic_cast<const AstLogical*>(node))
            {
                oss << "(";
                emit_node_(l->left.get(), oss);
                oss << " " << l->op << " ";
                emit_node_(l->right.get(), oss);
                oss << ")";
            }
            else
                throw std::runtime_error("unknown ast node");
        }

        void collect_variables(const AstNodeBase* node, std::unordered_map<std::string,std::string>& vars)
        {
            //目前只支持int
            if (auto* c = dynamic_cast<const AstCompare*>(node))
            {
                std::string name =  c->lhs;
                if (!vars.count(name))
                    vars[name] = "      int";
                return;
            }
            if (auto* l = dynamic_cast<const AstLogical*>(node))
            {
                collect_variables(l->left.get(), vars);
                collect_variables(l->right.get(), vars);
            }
        }

        static std::string generate_var_decls(const std::unordered_map<std::string,std::string>& vars)
        {
            std::ostringstream oss;
            for (auto& [name, type] : vars)
                oss << type << " " << name << " = 0;\n";
            return oss.str();
        }
    };

    // ---------------------- Parser ----------------------
    class Parser
    {
    public:
        Parser(const std::string& input, Graph& graph) : _cur(input.c_str()), _graph(graph) {}
        int parse_expression() { return parse_or(); }

    private:
        const char* _cur;
        Graph& _graph;

        void skip_whitespace() { while (isspace(*_cur)) ++_cur; }

        bool match(const char* s)
        {
            skip_whitespace();
            const char* p = _cur;
            while (*s && *p == *s) { ++p; ++s; }
            if (*s == 0) { _cur = p; return true; }
            return false;
        }

        std::string parse_identifier()
        {
            skip_whitespace();
            const char* start = _cur;
            while (isalnum(*_cur) || *_cur=='_') ++_cur;
            return std::string(start, _cur);
        }

        int parse_number()
        {
            skip_whitespace();
            int value = 0;
            while (isdigit(*_cur)) { value = value*10 + (*_cur - '0'); ++_cur; }
            return value;
        }

        int parse_compare()
        {
            std::string lhs = parse_identifier();
            skip_whitespace();
            std::string op;
            if (match(">="))      op = ">=";
            else if (match("<=")) op = "<=";
            else if (match("==")) op = "==";
            else if (match(">"))  op = ">";
            else if (match("<"))  op = "<";
            else
                throw std::runtime_error("unknown operator!");
            int rhs = parse_number();
            return _graph.add_compare_node(lhs, op, rhs);
        }

        int parse_and()
        {
            int left = parse_compare();
            while (match("&&"))
            {
                int right = parse_compare();
                left = _graph.add_logic_node("&&", {left, right});
            }
            return left;
        }

        int parse_or()
        {
            int left = parse_and();
            while (match("||"))
            {
                int right = parse_and();
                left = _graph.add_logic_node("||", {left, right});
            }
            return left;
        }
    };

    namespace _details
    {
        struct _node;//节点类型
        struct _socket;//插槽
        struct _edge;//连接线

        //当前的节点类型
        enum class _node_type
        {
            _edit,
            _result,
            _logic
        };

        struct _edge
            :QGraphicsPathItem
        {
            explicit _edge(_socket*_f);
            void  _set_to(_socket*);//设置终点端口
            void _update();
            ~_edge()override;
            _socket*_from = nullptr;//起点
            _socket*_to = nullptr;//终点
            QPointF _temp_end;//临时终点坐标，处理连接中断的情况
        };

        struct _socket
            :QGraphicsItem
        {
            explicit _socket(bool ,QGraphicsItem * parent = nullptr);

            ~_socket()override
            {
                while (_edges.empty()==false)
                    delete _edges.front();
            }

            void _connect(_edge*e){this->_edges.push_back(e);}
            void _disconnect(_edge*e)
            {
                if (const auto iter = std::find(_edges.begin(),_edges.end(),e);iter!=_edges.end())
                    this->_edges.erase(iter);
            }
            _node*_parent;
            bool _input = true;//是否是输入端口，默认是输出端口
            static constexpr qreal _socket_width = 8;//默认端口的大小
            std::vector<_edge*> _edges;
            void paint(QPainter *painter,const QStyleOptionGraphicsItem*, QWidget*) override;
            [[nodiscard]] QRectF boundingRect() const override{ return {-_socket_width, -_socket_width, _socket_width*2, _socket_width*2}; }
            QVariant itemChange(GraphicsItemChange,const QVariant&) override;
        };

        //static node
        struct _node final
            :QGraphicsObject
        {
            explicit _node(const QString&_string,QGraphicsItem*parent = nullptr);

            void _add_port(bool _is_input);//添加端口
            std::vector<_socket*>_input_ports;
            std::vector<_socket*>_output_ports;
            QString _name;
            QString _text;

            _node_type _type;
            ~_node() override;
        protected:
            QVariant itemChange(GraphicsItemChange,const QVariant&) override;
            [[nodiscard]] QRectF boundingRect() const override { return {0, 0, 160, 100}; }
            void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
        };

        _node::~_node()
        {
            for (const auto &_x:_input_ports)
                delete _x;

            for (const auto &_x:_output_ports)
                delete _x;
        }

        _node::_node(const QString &_string, QGraphicsItem *parent)
            :_name{_string},QGraphicsObject(parent)
        {
            this->setFlags(ItemIsMovable|ItemIsSelectable|ItemSendsGeometryChanges|ItemSendsGeometryChanges);
            this->_input_ports.reserve(4);
            this->_output_ports.reserve(4);
            _add_port(true);
            _add_port(false);
            _add_port(true);
            _add_port(false);
        }

        QVariant _node::itemChange(const GraphicsItemChange change,const QVariant&data)
        {
            if (change == ItemPositionChange && scene())
            {
                constexpr int grid_size = 25;
                const QPointF p =data.toPointF();
                return QPointF(std::round(p.x()/grid_size)*grid_size, std::round(p.y()/grid_size)*grid_size);
            }
            return QGraphicsItem::itemChange(change, data);
        }

        void _node::_add_port(bool _is_input)
        {
            const auto _rect = this->boundingRect();
            if (_is_input)
            {
                this->_input_ports.push_back(new _socket(_is_input,this));
                for (int index = 0;index<_input_ports.size();index++)
                {
                    this->_input_ports[index]->setPos(0,_rect.height()/static_cast<qreal>(_input_ports.size() + 1)*(index+1));
                    this->_input_ports[index]->_parent =this;
                }
            }
            else
            {
                this->_output_ports.push_back(new _socket(_is_input,this));
                for (int index = 0;index<_output_ports.size();index++)
                {
                    this->_output_ports[index]->setPos(_rect.width(),_rect.height()/static_cast<qreal>(_output_ports.size() + 1)*(index+1));
                    this->_output_ports[index]->_parent =this;
                }
            }
        }

        void _node::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
        {
            painter->setBrush(QColor(45, 45, 45));
            painter->setPen(QPen(Qt::lightGray, 1));
            painter->drawRoundedRect(boundingRect(), 8, 8);
            // 标题栏
            painter->setBrush(QColor(70, 70, 70));
            painter->drawRoundedRect(0, 0, 160, 25, 8, 8);
            painter->drawText(QRectF(0, 0, 160, 25), Qt::AlignCenter, this->_name);
        }

        _edge::_edge(_socket *_f)
            :_from{_f}
        {
            this->setEnabled(true);
            this->setFlags(ItemIsSelectable);
            this->setZValue(-1);            //必须设置。不然重绘其它内容会覆盖
            this->setPen(this->isSelected()?QPen(Qt::red, 3):QPen(QColor(0, 255, 255), 3));
        }

        _edge::~_edge()
        {
            if (_from)
                _from->_disconnect(this);
            if (_from)
                _from->_disconnect(this);
        }
        //绘制动态的贝塞尔曲线连接线
        void _edge::_update()
        {
            if (this->_from==nullptr)
                return;
            //计算起始坐标与终点坐标
            const auto _begin = this->_from->scenePos();
            const auto _end = _to?this->_to->scenePos():_temp_end;

            const qreal _dx = _end.x() - _begin.x();
            QPainterPath _path(_begin);
            _path.cubicTo(_begin.x() + _dx * 0.4, _begin.y(),_end.x() - _dx * 0.4, _end.y(),_end.x(), _end.y());
            this->setPath(_path);
        }

        void _edge::_set_to(_socket*_x)
        {
            this->_to = _x;
            this->_to->_connect(this);
            this->_update();
        }

        // Socket 实现
        _socket::_socket(bool _is_input,QGraphicsItem* parent)
            : QGraphicsItem(parent), _input(_is_input)
        {
            this->setFlag(ItemSendsScenePositionChanges);
        }

        void _socket::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
        {
            painter->setBrush(this->_input ? QColor(255, 200, 0) : QColor(255, 50, 50));
            painter->drawEllipse(-7, -7, 14, 14);
        }

        QVariant _socket::itemChange(GraphicsItemChange change, const QVariant &value)
        {
            if (change == ItemScenePositionHasChanged)
                for (const auto  e : this->_edges)
                    e->_update();
            return QGraphicsItem::itemChange(change, value);
        }

            //节点编辑对话框
    class _node_text_dialog
            : public QDialog
    {
        Q_OBJECT
    public:
        explicit _node_text_dialog(QWidget* parent = nullptr);

        void set_text(const QString& text);
        QString text() const;

    protected:
        void showEvent(QShowEvent* event) override;

    private:
        void _start_popup_animation();

    public:

        QLabel*_title;
        QWidget* _card = nullptr;
        QLineEdit* _edit = nullptr;
        bool _firstShow = true;
        QString _text;
    };

    _node_text_dialog::_node_text_dialog(QWidget* parent)
        : QDialog(parent)
    {
        this->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        setAttribute(Qt::WA_TranslucentBackground);
        this->setModal(true);

        setFixedSize(380, 200);

        _card = new QWidget(this);
        _card->setObjectName("card");

        //添加阴影
        auto* shadow = new QGraphicsDropShadowEffect(_card);
        shadow->setBlurRadius(24);
        shadow->setOffset(0, 6);
        shadow->setColor(QColor(0, 0, 0, 80));
        _card->setGraphicsEffect(shadow);

        _edit = new QLineEdit(_card);
        _edit->setMinimumHeight(40);
        _edit->setFont(QFont("Segoe UI", 10));

        auto* _ok_button = new QPushButton("ok", _card);
        auto* _cancel_button = new QPushButton("cancel", _card);

        connect(_ok_button, &QPushButton::clicked, this, &QDialog::accept);
        connect(_cancel_button, &QPushButton::clicked, this, &QDialog::reject);

        auto* btn_layout = new QHBoxLayout;
        btn_layout->addStretch();
        btn_layout->addWidget(_cancel_button);
        btn_layout->addWidget(_ok_button);

        auto* layout = new QVBoxLayout(_card);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->addWidget(_edit);
        layout->addLayout(btn_layout);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(20, 20, 20, 20);
        root->addWidget(_card);

        setStyleSheet(R"(
            #card {
                background: #2b2b2b;
                border-radius: 12px;
            }
            QLineEdit {
                padding: 0 10px;
                border-radius: 6px;
                border: 1px solid #3a3a3a;
                background: #1e1e1e;
                color: #ffffff;
            }
            QLineEdit:focus {
                border: 1px solid #3daee9;
            }
            QPushButton {
                min-width: 72px;
                height: 30px;
                border-radius: 6px;
                background: #3daee9;
                color: white;
                border: none;
            }
            QPushButton:hover {
                background: #5cc2ff;
            }
            QPushButton:pressed {
                background: #2b8bc6;
            }
        )");
    }

    void _node_text_dialog::set_text(const QString& text)
    {
        _edit->setText(text);
        _edit->selectAll();
    }

    QString _node_text_dialog::text() const
    {
        return _edit->text();
    }

    void _node_text_dialog::showEvent(QShowEvent* event)
    {
        QDialog::showEvent(event);

        _edit->setPlaceholderText(QStringLiteral("请输入")+_text);
        if (_firstShow)
        {
            _firstShow = false;
            _start_popup_animation();
        }
    }

    void _node_text_dialog::_start_popup_animation()
    {
        setWindowOpacity(0.0);

        const QRect end_rect = geometry();
        QRect startRect(
            end_rect.center().x() - end_rect.width() * 0.4,
            end_rect.center().y() - end_rect.height() * 0.4,
            end_rect.width() * 0.8,
            end_rect.height() * 0.8
        );

        auto* geo = new QPropertyAnimation(this, "geometry");
        geo->setDuration(180);
        geo->setStartValue(startRect);
        geo->setEndValue(end_rect);
        geo->setEasingCurve(QEasingCurve::OutBack);

        auto* opacity = new QPropertyAnimation(this, "windowOpacity");
        opacity->setDuration(140);
        opacity->setStartValue(0.0);
        opacity->setEndValue(1.0);

        auto* group = new QParallelAnimationGroup(this);
        group->addAnimation(geo);
        group->addAnimation(opacity);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }
    }

    // 画布视图：实现滚轮缩放 + 拖拽接收
    class NodeView
        : public QGraphicsView
    {
        Q_OBJECT
        Q_PROPERTY(bool m_grid_enabled   READ _is_grid_enabled WRITE _set_grid_enabled)
    public:
        explicit NodeView(QGraphicsScene* s)
            : QGraphicsView(s)
        {
            setAcceptDrops(true);
            //setDragMode(QGraphicsView::ScrollHandDrag);
            setRenderHint(QPainter::Antialiasing);
            // 核心：以鼠标为中心缩放
            setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
            setResizeAnchor(QGraphicsView::AnchorUnderMouse);
            viewport()->setStyleSheet("background: transparent;");

            this->scene()->setSceneRect(-5000, -5000, 10000, 10000);
            // 确保滚动条在需要时出现
            setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }


        bool _is_grid_enabled() const { return m_grid_enabled; }

        void _set_grid_enabled(bool enabled)
        {
            if (m_grid_enabled == enabled)
                return;
            m_grid_enabled = enabled;
            viewport()->update(); // 触发重绘
        }
    protected:
        // 缩放动画后删除
        static void delete_item(_details::_node*item,const  int duration =250)
        {
            if (!item) return;

            // 创建缩放动画
            auto *remove_animation = new QPropertyAnimation(item, "scale");
            remove_animation->setDuration(duration);
            remove_animation->setStartValue(1.0);
            remove_animation->setEndValue(0.0);
            remove_animation->setEasingCurve(QEasingCurve::OutBack);

            // 动画结束后删除
            QObject::connect(remove_animation, &QPropertyAnimation::finished, [item]()
            {
                if (item->scene())
                    item->scene()->removeItem(item);
                delete item;
            });

            remove_animation->start(QAbstractAnimation::DeleteWhenStopped);
        }

        void wheelEvent(QWheelEvent *event) override
        {
            constexpr double scale_factor = 1.15;
            if (event->angleDelta().y() > 0) {
                scale(scale_factor, scale_factor);
            }
            else
                this->scale(1.0 / scale_factor, 1.0 / scale_factor);
            event->accept();
        }

        void dragEnterEvent(QDragEnterEvent *e) override { e->acceptProposedAction(); }
        void dragMoveEvent(QDragMoveEvent *e) override { e->acceptProposedAction(); }
        void dropEvent(QDropEvent *e) override
        {
            if (e->mimeData()->hasText())
            {
                auto const  node = new _details::_node(e->mimeData()->text());

                if (e->mimeData()->text() == "逻辑节点")
                    node->_type = _details::_node_type::_logic;
                else if (e->mimeData()->text() == "文本节点")
                    node->_type = _details::_node_type::_edit;
                else if (e->mimeData()->text() == "输出节点")
                    node->_type = _details::_node_type::_result;

                qDebug()<<"add node :"<<e->mimeData()->text();
                scene()->addItem(node);
                QPointF p = mapToScene(e->pos());
                node->setPos(std::round(p.x()/25)*25, std::round(p.y()/25)*25);
                e->acceptProposedAction();
            }
        }
    protected:
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;

    protected:
        void drawBackground(QPainter *p, const QRectF &r) override
        {
            const auto background_color = QColor(25, 25, 25);
            //设置背景颜色
            p->fillRect(r, background_color);
            if (m_grid_enabled)
            {
                const auto grid_color = QColor(40, 40, 40);
                p->fillRect(r, background_color);
                p->setPen(QPen(grid_color, 1));
                for (qreal x = std::floor(r.left()/25)*25; x < r.right(); x+=25)
                    p->drawLine(x, r.top(), x, r.bottom());
                for (qreal y = std::floor(r.top()/25)*25; y < r.bottom(); y+=25)
                    p->drawLine(r.left(), y, r.right(), y);
            }

        }
    public:

        _details::_edge* _current_edge = nullptr;

        bool m_grid_enabled = false;
        std::vector<QString> _texts{};
        QString _text;
    };

    void NodeView::mousePressEvent(QMouseEvent *event)
    {
        if (event->button()==Qt::LeftButton)
        {
            //查找编辑器中的输出端口
            if (auto _socket_item = dynamic_cast<_details::_socket*>(this->itemAt(event->pos())))
            {
                if (_socket_item->_input == false)
                {
                    this->_current_edge = new _details::_edge{_socket_item};
                    this->_current_edge->_from = _socket_item;
                    this->_current_edge->_from->_connect(_current_edge);
                    this->scene()->addItem(_current_edge);
                }
            }
        }

        else if (event->button() == Qt::RightButton)
        {
            //查找编辑器中的所有的节点
            if (auto _node_item = dynamic_cast<_details::_node*>(this->itemAt(event->pos())))
            {
                QMenu menu(this);
                // 添加菜单项
                menu.addSeparator();  // 分隔线
                const QAction *delete_action = menu.addAction("删除");
                menu.addSeparator();
                if (_node_item->_type != _details::_node_type::_result)
                {
                    const auto edit_action = menu.addAction("编辑");
                    connect(edit_action, &QAction::triggered, [&]
                                    {
                                        if (!_node_item)
                                            return;

                                        _details::_node_text_dialog dlg(nullptr);
                                        if (_node_item->_type==_details::_node_type::_edit)
                                            dlg._text = "(例:x>=100,y<90)";
                                        else if (_node_item->_type==_details::_node_type::_logic)
                                            dlg._text = "||或者&&";
                                        dlg.set_text(_node_item->_text);

                                        if (dlg.exec() != QDialog::Accepted)
                                            return;

                                        _node_item->_text = dlg.text();

                                        if (_node_item->_type == _details::_node_type::_edit)
                                            qDebug() << "edit node add" << _node_item->_text;
                                        else if (_node_item->_type == _details::_node_type::_logic)
                                            qDebug() << "logic node add" << _node_item->_text;
                                        else
                                            qDebug() << "result node add" << _node_item->_text;
                                    });

                    connect(delete_action, &QAction::triggered, [&]
                    {
                        this->delete_item(_node_item);
                    });

                    connect(delete_action, &QAction::triggered, [&]
                    {
                        this->delete_item(_node_item);
                    });
                }

                // 显示菜单并获取选择
                QAction *selected = menu.exec(event->globalPos());

            }
        }
        return QGraphicsView::mousePressEvent(event);
    }

    void NodeView::mouseMoveEvent(QMouseEvent *event)
    {
        if (event->buttons() == Qt::LeftButton)
        {
            if (this->_current_edge != nullptr)
            {
                this->_current_edge->_temp_end = this->mapToScene(event->pos());
                this->_current_edge->_update();//
            }
        }
        return QGraphicsView::mouseMoveEvent(event);
    }

    void NodeView::mouseReleaseEvent(QMouseEvent *event)
    {
        if (this->_current_edge != nullptr)
        {
            auto _socket_item = dynamic_cast<_details::_socket*>(this->itemAt(event->pos()));
            if (_socket_item&&_socket_item->_input )//只能是逻辑节点的输入端口
            {
                static int   index =1;
                _current_edge->_set_to(_socket_item);
                //x>=10 &&v<100


                if (index ==1)
                    this->_text = _current_edge->_from->_parent->_text+_current_edge->_to->_parent->_text,++index;
                else
                    this->_text+= _current_edge->_from->_parent->_text+_current_edge->_to->_parent->_text;

                qDebug()<<"merge text is :"<<this->_text;
            }
            else
                delete this->_current_edge;//无效连接
            _current_edge = nullptr;
        }
        return QGraphicsView::mouseReleaseEvent(event);;
    }

    //节点侧边栏
    class NodeListWidget
        : public QListWidget
    {
    public:
        NodeListWidget()
        {
            addItems({"逻辑节点", "文本节点", "输出节点"});
            setDragEnabled(true);
        }
    protected:
        void mousePressEvent(QMouseEvent *e) override {
            _start_pos = e->pos();
            QListWidget::mousePressEvent(e);
        }

        void mouseMoveEvent(QMouseEvent *e) override
        {
            if (!(e->buttons() & Qt::LeftButton))
                return;
            if ((e->pos() - _start_pos).manhattanLength() < QApplication::startDragDistance())
                return;
            QListWidgetItem *it = itemAt(_start_pos);
            if (it)
            {
                auto *drag = new QDrag(this);
                auto *m = new QMimeData;
                m->setText(it->text());
                drag->setMimeData(m);

                // 設置拖拽時的縮略圖
                QPixmap pixmap(100, 30);
                pixmap.fill(QColor(0, 191, 255, 100));
                drag->setPixmap(pixmap);
                drag->exec(Qt::CopyAction);
            }
        }
    private:
        QPoint _start_pos;
    };

    class MainWindow
        : public QMainWindow
    {

        Q_OBJECT
    public:

        void gen_cpp_code()
        {
            qDebug()<<this->view->_text;

            this->view->_text.chop(2);
            NodeEditor::Graph graph;

            // 解析生成 Graph
            NodeEditor::Parser parser(view->_text.toStdString(), graph);
            int root_id = parser.parse_expression();

            // Graph -> AST
            NodeEditor::AstBuilder builder(graph);
            std::unique_ptr<NodeEditor::AstNodeBase> ast = builder.build(root_id);

            // AST -> C++ 表达式/文件
            NodeEditor::CppEmitter emitter;
            std::string expr_str = emitter.emit_expr(ast.get());
            cpp_code = emitter.emit_cpp_code(ast.get());

            // 写入文件
            std::ofstream ofs("cpp_generated.cpp");
            ofs << cpp_code;
            ofs.close();

            qDebug() << "C++ code generated to cpp_generated.cpp\n";

            auto string = expr_str.c_str();
            qDebug() << "Expression string: " << string<< "\n";
        }

        MainWindow()
        {
                const QString _node_editor_qss = R"(
            QMainWindow { background-color: #1a1a1a; }
            QSplitter::handle { background-color: #2d2d2d; }
                QSplitter::handle:horizontal { width: 2px; }
            QListWidget {
            outline:0;
                background-color: #1e1e2e;  /* 深色背景 */
                border: 1px solid #3b3b5a; /* 边框 */
                border-radius: 5px;
                padding: 0px;               /* 去掉内边距，让横线对齐 */
            }

        /* ------------------- QListWidgetItem ------------------- */
        QListWidget::item {
            background-color: #1e1e2e;  /* 默认背景 */
            color: #d4d4d4;             /* 字体颜色 */
            padding: 8px 12px;
            border-bottom: 1px solid #2c2c44; /* 横线分割 */
        }

        /* 悬停效果 */
        QListWidget::item:hover {
            background-color: #2c2c44;
        }

        /* 选中效果 */
        QListWidget::item:selected {
            background-color: #007acc;  /* VSCode 蓝色选中 */
            color: #ffffff;
        }

        /* ------------------- 滚动条 ------------------- */
        QScrollBar:vertical {
            background: #1e1e2e;
            width: 10px;
            margin: 0px 0px 0px 0px;
            border-radius: 5px;
        }

        QScrollBar::handle:vertical {
            background: #3b3b5a;
            min-height: 20px;
            border-radius: 5px;
        }

        QScrollBar::handle:vertical:hover {
            background: #007acc;
        }

        QScrollBar::sub-line:vertical, QScrollBar::add-line:vertical {
            height: 0px;  /* 隐藏箭头 */
        }

        QScrollBar::add-page, QScrollBar::sub-page {
            background: none;
        }

        )";
            setStyleSheet(_node_editor_qss);
            this->setWindowTitle("Node Editor Studio");
            QSplitter* splitter = new QSplitter(Qt::Horizontal);
            splitter->setHandleWidth(2);

            lib = new NodeListWidget();
            view = new NodeView(new QGraphicsScene());

            prop = new QWidget();
            prop->setObjectName("PropPanel");
            prop->setStyleSheet("#PropPanel { background: #212121; border-left: 1px solid #333; }");
            prop->setMinimumWidth(50); // 允许缩得很小
            QPushButton*button = new QPushButton{"代码生成",prop};
            const auto grid_button = new QPushButton{"禁用/启用网格",prop};
            grid_button->setCheckable(true);
            button->resize(200,100);
            const auto button_layout = new QVBoxLayout;
            connect(button,&QPushButton::clicked,this,&MainWindow::gen_cpp_code);
            button_layout->addWidget(button);
            button_layout->addWidget(grid_button);
            button_layout->addStretch();
            connect(grid_button, &QPushButton::toggled, view, &NodeView::_set_grid_enabled);
            prop->setLayout(button_layout);

            splitter->addWidget(lib);
            splitter->addWidget(view);
            splitter->addWidget(prop);

            splitter->setStretchFactor(0, 0);
            splitter->setStretchFactor(1, 1); // 中间画布最贪婪
            splitter->setStretchFactor(2, 0);

            // 设置初始化大小 (侧边栏 200, 画布 800, 属性栏 150)
            splitter->setSizes(QList<int>({200, 800, 150}));

            setCentralWidget(splitter);
            resize(1200, 800);
        }

        NodeListWidget*lib;
        NodeView* view ;
        QWidget* prop;
        std::string cpp_code;
    };
}
// ---------------------- Graph ----------------------

#include "module3.moc"

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    QApplication a(argc, argv);
    NodeEditor::MainWindow w;
    w.show();
    return a.exec();
}
