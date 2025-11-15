#include <iostream>
#include <memory>
#include <linux/can.h>
#include <list>

class observer;
class subject
{
public:
    subject(){}
    virtual ~subject() = default;
    virtual void attach(const std::shared_ptr<observer> pobserver) = 0;
    virtual void detach(const std::shared_ptr<observer> pobserver) = 0;
    virtual void notify() = 0;
protected:
    std::list<std::weak_ptr<observer>> m_pobserver_list;
};


class observer
{
public:
    observer(const std::string &name = "unkown"):m_namestr(name){}
    virtual ~observer() = default;
    virtual void update(subject * sbj) = 0;
    virtual const std::string &name(){return m_namestr;}
protected:
    std::string m_namestr;
};

class manager;

class dc_unit : public subject
{
public:
    dc_unit(int id, int value):m_id(id), m_value(value){}
    ~dc_unit(){}
    void set_value(int value)
    {
        m_value = value;
        notify();
    }

    int get_id(){return m_id;}
    int get_value(){return m_value;}

    void attach(std::shared_ptr<observer> obs) override
    {
        m_pobserver_list.push_back(obs);
    }

    void detach(std::shared_ptr<observer> obs) override
    {
        m_pobserver_list.remove_if([&obs](const std::weak_ptr<observer> &wptr){ return wptr.lock() == obs;});
    }

    void notify() override
    {
        for (auto it = m_pobserver_list.begin(); it != m_pobserver_list.end();)
        {
            if (auto sptr = it->lock())
            {
                sptr->update(this);
                it++;
            }
            else
            {
                it = m_pobserver_list.erase(it);
            }
        }
    }

private:
    int m_id;
    int m_value;
};

class manager : public observer
{
public:
    manager():observer("manager"){}

    void update(subject* subject)
    {
        auto *dc = dynamic_cast<dc_unit *>(subject);
        if (dc)
        {
            std::cout << name() << " received update from dcunit[" << dc->get_id() << "], value = " << dc->get_value() << std::endl;
        }
    }
};


int main(void)
{
    // test();
    auto m = std::make_shared<manager>();

    dc_unit dcdc[12]{dc_unit(1, 1), dc_unit(2, 2), dc_unit(3, 3),
                     dc_unit(4, 4), dc_unit(5, 5), dc_unit(6, 6),
                     dc_unit(7, 7), dc_unit(8, 8), dc_unit(9, 9),
                     dc_unit(10, 10), dc_unit(11, 11), dc_unit(12, 12)
    };

    for (int i = 0; i < 12; i++)
    {
        dcdc[i].attach(m);
    }

    dcdc[0].set_value(1111);
    dcdc[6].set_value(7777);
    return 0;
}

/* g++ test.cpp -g -o test && ./test */
