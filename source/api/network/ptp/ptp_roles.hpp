#ifndef PTP_ROLES_HPP
#define PTP_ROLES_HPP

enum class PtpRole {
    SLAVE,  // A normal vehicle gateway that synchronizes its clock
    MASTER  // The RSU that acts as the time source
};

#endif