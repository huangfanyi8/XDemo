

#ifndef SWITCH_BUTTON_AST_HPP
#define SWITCH_BUTTON_AST_HPP

#include<string>
#include<iostream>
#include<vector>

namespace AST
{
    enum class _node_type
    {

        _sequence,//顺序执行的语句
        _if,//if 语句
        _while//while语句
    };

    struct _node
    {
        std::vector<_node*>_next;
        std::string _content;
        _node_type _type;
        bool _cond = false;//执行语句
    };

    inline void print(_node*head)
    {
        _node*_cur = head;
        while (_cur)
        {
            std::cout << _cur->_content << std::endl;
            _cur = _cur->_next.empty()?nullptr:_cur->_next[0];
        }
    }

}
#endif //SWITCH_BUTTON_AST_HPP