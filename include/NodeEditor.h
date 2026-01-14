#ifndef NODEEIDITOR_NODEEDITOR_H
#define NODEEIDITOR_NODEEDITOR_H
#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QMouseEvent>
#include <QDebug>
namespace dong_dong_widgets::node_editor
{
    namespace _details
    {
        struct _node;//节点类型
        struct _socket;//插槽
        struct _edge;//连接线

        struct _edge
            :QGraphicsPathItem
        {
            explicit _edge(_socket*_f);
            void  _set_to(_socket*);//设置终点端口
            void _update();
            ~_edge()override;
            _socket*_from = nullptr;//起点
            _socket*_to = nullptr;//终点
            QPointF _temp_end;//临时终点坐标，用作于连接失败的情况
        };

        template<class T,bool Input>
        struct _socket_1
            :T
        {

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
            bool _input = true;//是否是输入端口，默认是输出端口
            static constexpr qreal _socket_width = 8;

            std::vector<_edge*> _edges;
            void paint(QPainter *painter,const QStyleOptionGraphicsItem*, QWidget*) override;
            [[nodiscard]] QRectF boundingRect() const override{ return {-_socket_width, -_socket_width, _socket_width*2, _socket_width*2}; }
            QVariant itemChange(GraphicsItemChange,const QVariant&) override;
        };

        //static node
        struct _node
            :QGraphicsObject
        {
            explicit _node(const QString&_string,QGraphicsItem*parent = nullptr);

            void _add_port(bool _is_input);//添加端口
            std::vector<_socket*>_input_ports;
            std::vector<_socket*>_output_ports;
            QString _name;

            ~_node() override;
        protected:
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

        void _node::_add_port(bool _is_input)
        {
            const auto _rect = this->boundingRect();
            if (_is_input)
            {
                this->_input_ports.push_back(new _socket(_is_input,this));
                for (int index = 0;index<_input_ports.size();index++)
                {
                    this->_input_ports[index]->setPos(0,_rect.height()/static_cast<qreal>(_input_ports.size() + 1)*(index+1));
                }
            }
            else
            {
                this->_output_ports.push_back(new _socket(_is_input,this));
                for (int index = 0;index<_output_ports.size();index++)
                {
                    this->_output_ports[index]->setPos(_rect.width(),_rect.height()/static_cast<qreal>(_output_ports.size() + 1)*(index+1));
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
    }

    class NodeEditor
        :public QGraphicsView
    {
    public:
        explicit NodeEditor(QGraphicsScene* s);

    protected:
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;
    private:
        _details::_edge* _current_edge = nullptr;
    };

    NodeEditor::NodeEditor(QGraphicsScene *s)
        : QGraphicsView(s)
    {
        this->setRenderHint(QPainter::Antialiasing);
        this->setBackgroundBrush(QColor(25, 25, 25));//TODO 颜色可以设置为动态
        this->setSceneRect(-5000, -5000, 10000, 10000);
    }

    void NodeEditor::wheelEvent(QWheelEvent *event)
    {
        const double factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
        this->scale(factor, factor);
    }

    void NodeEditor::mousePressEvent(QMouseEvent *event)
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
        return QGraphicsView::mousePressEvent(event);
    }

    void NodeEditor::mouseMoveEvent(QMouseEvent *event)
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

    void NodeEditor::mouseReleaseEvent(QMouseEvent *event)
    {
        if (this->_current_edge != nullptr)
        {
            auto _socket_item = dynamic_cast<_details::_socket*>(this->itemAt(event->pos()));
            if (_socket_item&&_socket_item->_input )//只能是输入端口
            {_current_edge->_set_to(_socket_item);}
            else
                delete this->_current_edge;;//无效连接
            _current_edge = nullptr;
        }
        return QGraphicsView::mouseReleaseEvent(event);;
    }

}

#endif //NODEEIDITOR_NODEEDITOR_H