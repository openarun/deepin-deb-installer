/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "packageslistdelegate.h"
#include "deblistmodel.h"
#include "utils.h"

#include <DSvgRenderer>
#include <DPalette>
#include <DStyleHelper>
#include <DApplicationHelper>

#define THEME_DARK 2//"dark"
#define THEME_LIGHT 1//"light"

DWIDGET_USE_NAMESPACE

PackagesListDelegate::PackagesListDelegate(QAbstractItemView *parent)
    : DStyledItemDelegate(parent)
    , m_parentView(parent)
{

    const QIcon icon = QIcon::fromTheme("application-x-deb");
    const auto ratio = qApp->devicePixelRatio();
    m_packageIcon = icon.pixmap(32, 32);

    m_removeIcon = Utils::renderSVG(":/images/active_tab_close_normal.svg", QSize(16, 16));
    m_removeIcon.setDevicePixelRatio(ratio);

    m_view= reinterpret_cast<PackagesListView*>(parent);
}

void PackagesListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    if (index.isValid()) {
        painter->save();
        painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform, true);

        const int content_x = 46;
        DPalette pa = DApplicationHelper::instance()->palette(m_parentView);

        //绘制分割线
        QRect lineRect;
        lineRect.setX(content_x);
        lineRect.setY(option.rect.y()+48-1);
        lineRect.setWidth(option.rect.width()-content_x-10);
        lineRect.setHeight(1);

        DStyleHelper styleHelper;
        QColor fillColor = styleHelper.getColor(static_cast<const QStyleOption *>(&option), DPalette::Shadow);
        painter->fillRect(lineRect, fillColor);

        QRect bg_rect = option.rect;

        // draw package icon
        const int x = 6;
        int y = bg_rect.y()+7;
        painter->drawPixmap(x, y, m_packageIcon);

        // draw package name
        QRect name_rect = bg_rect;
        name_rect.setX(content_x);
        name_rect.setY(bg_rect.y()+6);
        name_rect.setHeight(20);

        const QString pkg_name = index.data(DebListModel::PackageNameRole).toString();
        QString mediumFontFamily = Utils::loadFontFamilyByType(Utils::SourceHanSansMedium);
        QString normalFontFamily = Utils::loadFontFamilyByType(Utils::SourceHanSansNormal);
        QString defaultFontFamily = Utils::loadFontFamilyByType(Utils::DefautFont);

        painter->setFont(Utils::loadFontBySizeAndWeight(mediumFontFamily, 14, QFont::Medium));
        const QString elided_pkg_name = painter->fontMetrics().elidedText(pkg_name, Qt::ElideRight, 306);
        const QRectF name_bounding_rect = painter->boundingRect(name_rect, elided_pkg_name, Qt::AlignLeft | Qt::AlignBottom);

        painter->setPen(styleHelper.getColor(static_cast<const QStyleOption *>(&option), DPalette::WindowText));
        painter->drawText(name_rect, pkg_name, Qt::AlignLeft | Qt::AlignVCenter);

        // draw package version
        QRect version_rect = name_rect;
        const int version_x = static_cast<int>(name_bounding_rect.right()) + 10;
        const int version_y = version_rect.top();
        version_rect.setLeft(version_x);
        version_rect.setTop(version_y);
        version_rect.setRight(option.rect.right() - 85);
        const QString version = index.data(DebListModel::PackageVersionRole).toString();
        const QString version_str = painter->fontMetrics().elidedText(version, Qt::ElideRight, version_rect.width());
        painter->setPen(styleHelper.getColor(static_cast<const QStyleOption *>(&option), DPalette::BrightText));
        painter->setFont(Utils::loadFontBySizeAndWeight(defaultFontFamily, 12, QFont::Light));
        painter->drawText(version_rect, version_str, Qt::AlignLeft | Qt::AlignVCenter);

        // install status
        const int operate_stat = index.data(DebListModel::PackageOperateStatusRole).toInt();
        if (operate_stat != DebListModel::Prepare) {
            QRect install_status_rect = option.rect;
            install_status_rect.setRight(option.rect.right() - 20);

            painter->setFont(Utils::loadFontBySizeAndWeight(mediumFontFamily, 11, QFont::Medium));
            switch (operate_stat) {
                case DebListModel::Operating:
                    painter->setPen(QColor("#7C7C7C"));
                    painter->drawText(install_status_rect, tr("Installing"), Qt::AlignVCenter | Qt::AlignRight);
                    break;
                case DebListModel::Success:
                    painter->setPen(QColor("#417505"));
                    painter->drawText(install_status_rect, tr("Installed"), Qt::AlignVCenter | Qt::AlignRight);
                    break;
                case DebListModel::Waiting:
                    painter->setPen(QColor("#7C7C7C"));
                    painter->drawText(install_status_rect, tr("Waiting"), Qt::AlignVCenter | Qt::AlignRight);
                    break;
                default:
                    painter->setPen(QColor("#FF6D6D"));
                    painter->drawText(install_status_rect, tr("Failed"), Qt::AlignVCenter | Qt::AlignRight);
                    break;
            }
        } else if (index.data(DebListModel::WorkerIsPrepareRole).toBool() &&
                   index.data(DebListModel::ItemIsCurrentRole).toBool()) {
            int icon_width = static_cast<int>(m_removeIcon.width() / m_removeIcon.devicePixelRatio());
            int icon_height = static_cast<int>(m_removeIcon.height() / m_removeIcon.devicePixelRatio());
            // draw remove icon
            const int x = option.rect.right() - icon_width - 18;
            const int y = option.rect.top() + (option.rect.height() - icon_height) / 2;
            painter->drawPixmap(x, y, m_removeIcon);
        }

        // draw package info
        QString info_str;
        QRect info_rect = option.rect;
        info_rect.setLeft(content_x);
        info_rect.setTop(name_rect.bottom()+2);

        const int install_stat = index.data(DebListModel::PackageVersionStatusRole).toInt();
        QColor penColor = styleHelper.getColor(static_cast<const QStyleOption *>(&option), pa, DPalette::TextTips);
        if (operate_stat == DebListModel::Failed) {
            info_str = index.data(DebListModel::PackageFailReasonRole).toString();
            penColor = QColor("#FF6D6D");
        } else if (install_stat != DebListModel::NotInstalled) {
            if (install_stat == DebListModel::InstalledSameVersion) {
                info_str = tr("Same version installed");
//                penColor = QColor("#417505");
            } else {
                info_str =
                    tr("Other version installed: %1").arg(index.data(DebListModel::PackageInstalledVersionRole).toString());
//                penColor = QColor("#FF6D6D");
            }
        } else {
            info_str = index.data(DebListModel::PackageDescriptionRole).toString();
        }

        painter->setPen(QPen(penColor));
        painter->setFont(Utils::loadFontBySizeAndWeight(normalFontFamily, 12, QFont::ExtraLight));
        info_str = painter->fontMetrics().elidedText(info_str, Qt::ElideRight, 306);
        painter->drawText(info_rect, info_str, Qt::AlignLeft | Qt::AlignTop);

        painter->restore();
    }
    else
    {
        DStyledItemDelegate::paint(painter, option, index);
    }
}

QSize PackagesListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);

    return index.data(Qt::SizeHintRole).toSize();
}
