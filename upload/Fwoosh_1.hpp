/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Fwoosh.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: juandrie <juandrie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/27 14:40:28 by juandrie          #+#    #+#             */
/*   Updated: 2024/06/27 14:44:02 by juandrie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include "ASpell.hpp"

class Fwoosh : public ASpell
{
    public :

        Fwoosh();
        ~Fwoosh();
        ASpell *clone() const;
};
