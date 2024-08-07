from django.urls import path

from . import views

urlpatterns = [
    path("", views.ping_pong, name="ping_pong"),
    path("punto_estrella", views.show_challenge_explanation, name="show_challenge_explanation"),
]
