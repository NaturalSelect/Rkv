#pragma once
#ifndef _RKV_MIGRATIONMANGER_HPP
#define _RKV_MIGRATIONMANGER_HPP



namespace rkv
{
    class MigrationManger
    {
    private:
        using Self = rkv::MigrationManger;
    
    public:
    
        MigrationManger();
    
        MigrationManger(const Self &other);
    
        MigrationManger(Self &&other) noexcept;
    
        inline Self &operator=(const Self &other)
        {
            Self tmp{other};
            std::swap(tmp,*this);
            return *this;
        }
    
        Self &operator=(Self &&other) noexcept;
    
        ~MigrationManger() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }
    };
}

#endif