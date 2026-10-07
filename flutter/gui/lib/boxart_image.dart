import 'package:cached_network_image/cached_network_image.dart';
import 'package:flutter/material.dart';

/// Box art shares the package's default disk cache, keyed by image URL.
class BoxArtImage extends StatelessWidget {
  const BoxArtImage({
    super.key,
    required this.url,
    required this.semanticLabel,
    this.iconSize = 64,
  });

  final String? url;
  final String semanticLabel;
  final double iconSize;

  @override
  Widget build(BuildContext context) => Semantics(
    label: semanticLabel,
    image: true,
    child: url == null
        ? Icon(Icons.videogame_asset, size: iconSize)
        : CachedNetworkImage(
            imageUrl: url!,
            fit: BoxFit.contain,
            memCacheWidth: 480,
            fadeInDuration: Duration.zero,
            fadeOutDuration: Duration.zero,
            placeholder: (_, _) =>
                Icon(Icons.image_outlined, size: iconSize),
            errorWidget: (_, _, _) =>
                Icon(Icons.broken_image_outlined, size: iconSize),
          ),
  );
}
