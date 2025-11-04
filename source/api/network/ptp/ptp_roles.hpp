#ifndef PTP_ROLES_HPP
#define PTP_ROLES_HPP

enum class PtpRole {
    SLAVE, // a normal vehicle gateway that syncronizes its clock
    MASTER // the RSU that acts as the time source
};

#endif