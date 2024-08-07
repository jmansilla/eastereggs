from django.contrib import admin

from .models import SOGroup, PingPong, Deadline


class DeadlineAdmin(admin.ModelAdmin):
    list_display = ('name', 'deadline')

admin.site.register(SOGroup)
admin.site.register(PingPong)
admin.site.register(Deadline, DeadlineAdmin)